#include "cthread_temp_controller_processcommbuf.h"
#include "ctemp_controller.h"

CThread_Temp_Controller_ProcessCommbuf::CThread_Temp_Controller_ProcessCommbuf(CTemp_Controller* parent, CQueue* queue)
{
    m_parent = parent;
    m_Queue = queue;

    Init();
}

CThread_Temp_Controller_ProcessCommbuf::~CThread_Temp_Controller_ProcessCommbuf()
{
}


/*
    override methods
*/
void CThread_Temp_Controller_ProcessCommbuf::run()
{
    ProcessCommbuf();
}

/*
    methods
*/
void CThread_Temp_Controller_ProcessCommbuf::Init()
{
    m_bFlag_Thread_Quit = false;
    m_bCheck_Thread_Quit = false;
}

void CThread_Temp_Controller_ProcessCommbuf::StopThread()
{
    m_bFlag_Thread_Quit = true;
    while(!m_bCheck_Thread_Quit){};
    quit();
    wait();
}

void CThread_Temp_Controller_ProcessCommbuf::ProcessCommbuf()
{
    quint8 TempBuf[MAXBUFSIZE]; // Temp Data array

    while(!m_bFlag_Thread_Quit)
    {
        msleep(50);

        uint16_t bufsize = m_Queue->GetDataLength();

        // 버퍼에 데이터가 최소 사이즈 만큼 안들어 왔을 경우 무시
        if(bufsize < 3) continue;

        uint16_t index = 0;
        uint8_t slave_addr = 0;
        uint8_t function_code = 0;
        uint8_t exception_code = 0;
        uint16_t frame_size = 0;

        memset(TempBuf, 0, bufsize * sizeof(uint8_t));
        index = m_Queue->GetTail();
        for(int i = 0; i < bufsize; i++)
        {
            TempBuf[i] = m_Queue->GetData(index++);
            index &= (MAXBUFSIZE - 1);
        }

        // Slave Address (Device ID)
        frame_size++;
        slave_addr = TempBuf[0];

        // Function Code
        frame_size++;
        function_code = TempBuf[1];

        // Data
        if(function_code > 0x80)        // Err
        {
            frame_size++;
            exception_code = TempBuf[2];
        }
        else
        {
            switch(function_code)
            {
            case TEMP_CONTROLLER_CMD_READ_COIL:
            {
                frame_size++;
                uint8_t data_len = TempBuf[2];
                frame_size += (uint16_t)(data_len);
            }
                break;

            case TEMP_CONTROLLER_CMD_READ_DISCRETE_INPUTS:
            {
                frame_size++;
                uint8_t data_len = TempBuf[2];
                frame_size += (uint16_t)(data_len);
            }
                break;

            case TEMP_CONTROLLER_CMD_READ_HOLDING_REGISTERS:
            {
                frame_size++;
                uint8_t data_len = TempBuf[2];
                frame_size += (uint16_t)(data_len);
            }
                break;

            case TEMP_CONTROLLER_CMD_READ_INPUT_REGISTER:
            {
                frame_size++;
                uint8_t data_len = TempBuf[2];
                frame_size += (uint16_t)(data_len);
            }
                break;

            case TEMP_CONTROLLER_CMD_WRITE_SINGLE_COIL:
            {
                frame_size += 4;
            }
                break;

            case TEMP_CONTROLLER_CMD_WRITE_SINGLE_REGISTER:
            {
                frame_size += 4;
            }
                break;

            case TEMP_CONTROLLER_CMD_WRITE_MULTIPLE_COILS:
            {
                frame_size += 4;
            }
                break;

            case TEMP_CONTROLLER_CMD_WRITE_MULTIPLE_REGISTERS:
            {
                frame_size += 4;
            }
                break;

            default:
                break;
            }
        }

        // crc 포함 프레임 사이즈 > 버퍼 사이즈 일 때 무시
        if(frame_size + sizeof(uint16_t) > bufsize) continue;

        // Crc Check
        uint16_t crc_data = 0, crc_calc = 0;
        memcpy_s(&crc_data, sizeof(uint16_t), TempBuf + frame_size, sizeof(uint16_t));
        crc_calc = CalCRC(TempBuf, frame_size);
        if(crc_data == crc_calc)
        {
            uint8_t monitor_num = m_parent->m_Hash_AddressToMonitor[slave_addr];

            // 데이터 복사
            memcpy_s(m_parent->m_STC_Comm_RecvData[monitor_num].Frame.Buf, frame_size, TempBuf, frame_size);

//            for(int i = 0; i < frame_size; i++)
//            {
//                qDebug("%x", m_parent->m_STC_Comm_RecvData[monitor_num].Frame.Buf[i]);
//            }

            // 데이터 파싱
            if(function_code > 0x80)
            {
                m_parent->m_STC_Comm_RecvData[monitor_num].Data.Err = m_parent->m_STC_Comm_RecvData[monitor_num].Frame.Buf[2];
            }
            else
            {
                m_parent->m_STC_Comm_RecvData[monitor_num].Data.Err = 0;
                switch(m_parent->m_Cmd_Status_RequestData[monitor_num])
                {
                case static_cast<quint16>(CMD_TEMPCONTROLLER_REQUESTDATA::SETPOINT):
                {
                    uint16_t sv = 0;
                    memcpy_s(&sv, sizeof(uint16_t), &TempBuf[4], sizeof(uint16_t));
                    m_parent->m_STC_Comm_RecvData[monitor_num].Data.SV = (float)(SWAP16(sv));
                    m_parent->m_Comm_Response_Check[monitor_num] = 1;
                }
                    break;

                case static_cast<quint16>(CMD_TEMPCONTROLLER_REQUESTDATA::GETPOINT):
                {
                    uint16_t sv = 0;
                    memcpy_s(&sv, sizeof(uint16_t), &TempBuf[3], sizeof(uint16_t));
                    m_parent->m_STC_Comm_RecvData[monitor_num].Data.SV = (float)(SWAP16(sv));
                    m_parent->m_Comm_Response_Check[monitor_num] = 1;
                }
                    break;

                case static_cast<quint16>(CMD_TEMPCONTROLLER_REQUESTDATA::GETMEAS):
                {
                    uint16_t pv = 0;
                    memcpy_s(&pv, sizeof(uint16_t), &TempBuf[3], sizeof(uint16_t));
                    m_parent->m_STC_Comm_RecvData[monitor_num].Data.PV = (float)(SWAP16(pv));
                }
                    break;

                default:
                    break;
                }

            }
        }

        frame_size += sizeof(uint16_t);

        m_Queue->Delete((uint16_t)frame_size);

    }

    m_bCheck_Thread_Quit = true;
}

uint16_t CThread_Temp_Controller_ProcessCommbuf::CalCRC(uint8_t* cBuf, uint8_t cLen)
{
    uint8_t cTmp;
    uint16_t nCrc = 0xffff;
    while (cLen--)
    {
        cTmp = *cBuf++ ^ nCrc;
        nCrc >>= 8;
        nCrc ^= MODBUS_CRC_16_TABLE[cTmp];
    }
    return nCrc;
}
