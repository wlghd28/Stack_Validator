#include "cthread_temp_controller_processcmdbuf.h"
#include "Stack_Validator.h"

CThread_Temp_Controller_ProcessCmdbuf::CThread_Temp_Controller_ProcessCmdbuf(CTemp_Controller* parent)
{
    m_parent = parent;

    Init();
}

CThread_Temp_Controller_ProcessCmdbuf::~CThread_Temp_Controller_ProcessCmdbuf()
{
}

/*
    override methods
*/
void CThread_Temp_Controller_ProcessCmdbuf::run()
{
    ProcessCmdStatus();
}

/*
    methods
*/
void CThread_Temp_Controller_ProcessCmdbuf::Init()
{
    m_bFlag_Thread_Quit = false;
    m_bCheck_Thread_Quit = false;
    for(int i = 0; i < LOADER_CHANNEL; i++)
    {
        m_Cmd_Status[i] = static_cast<quint16>(CMD_FLOWCONTROLLER_REQUESTDATA::NONE);
    }
}

void CThread_Temp_Controller_ProcessCmdbuf::StopThread()
{
    m_bFlag_Thread_Quit = true;
    while(!m_bCheck_Thread_Quit){};
    quit();
    wait();
}

void CThread_Temp_Controller_ProcessCmdbuf::ProcessCmdStatus()
{
    quint8 monitor_index = 0;

    while(!m_bFlag_Thread_Quit)
    {
        switch(m_Cmd_Status[monitor_index])
        {
        case static_cast<quint16>(CMD_TEMPCONTROLLER_REQUESTDATA::SETPOINT):
            if(m_parent->m_Comm_Response_Check[monitor_index])
            {
                m_parent->GetTempControllerMeas(monitor_index);
                m_parent->m_Cmd_Status_RequestData[monitor_index] = static_cast<quint16>(CMD_TEMPCONTROLLER_REQUESTDATA::GETMEAS);
                ProcessCmdbuf(TEMP_CONTROLLER_CMD_READ_INPUT_REGISTER, &(m_parent->m_STC_Comm_SendData[monitor_index]));
            }
            else
            {
                m_parent->m_Cmd_Status_RequestData[monitor_index] = static_cast<quint16>(CMD_TEMPCONTROLLER_REQUESTDATA::SETPOINT);
                ProcessCmdbuf(TEMP_CONTROLLER_CMD_WRITE_SINGLE_REGISTER, &(m_parent->m_STC_Comm_SendData[monitor_index]));
            }
            break;

        case static_cast<quint16>(CMD_TEMPCONTROLLER_REQUESTDATA::GETPOINT):
            if(m_parent->m_Comm_Response_Check[monitor_index])
            {
                m_parent->GetTempControllerMeas(monitor_index);
                m_parent->m_Cmd_Status_RequestData[monitor_index] = static_cast<quint16>(CMD_TEMPCONTROLLER_REQUESTDATA::GETMEAS);
                ProcessCmdbuf(TEMP_CONTROLLER_CMD_READ_INPUT_REGISTER, &(m_parent->m_STC_Comm_SendData[monitor_index]));
            }
            else
            {
                m_parent->m_Cmd_Status_RequestData[monitor_index] = static_cast<quint16>(CMD_TEMPCONTROLLER_REQUESTDATA::GETPOINT);
                ProcessCmdbuf(TEMP_CONTROLLER_CMD_READ_INPUT_REGISTER, &(m_parent->m_STC_Comm_SendData[monitor_index]));
            }
            break;

        case static_cast<quint16>(CMD_TEMPCONTROLLER_REQUESTDATA::GETMEAS):
            m_parent->m_Cmd_Status_RequestData[monitor_index] = static_cast<quint16>(CMD_TEMPCONTROLLER_REQUESTDATA::GETMEAS);
            ProcessCmdbuf(TEMP_CONTROLLER_CMD_READ_INPUT_REGISTER, &(m_parent->m_STC_Comm_SendData[monitor_index]));
            m_parent->ErrHandleTempController(monitor_index);
            if(m_parent->m_parent->m_ptrMonitor[monitor_index] == nullptr)
                m_Cmd_Status[monitor_index] = static_cast<quint16>(CMD_TEMPCONTROLLER_REQUESTDATA::NONE);
            break;

        default:
            break;
        }

        if(++monitor_index >= LOADER_CHANNEL)
            monitor_index = 0;

        msleep(TEMPCONTROLLER_REQUEST_TIMERCLOCK);
    }

    m_bCheck_Thread_Quit = true;
}

void CThread_Temp_Controller_ProcessCmdbuf::ProcessCmdbuf(quint16 Cmd, STC_COMM_TEMP_CONTROLLER_SEND* SendData)
{
    switch(Cmd)
    {
    case TEMP_CONTROLLER_CMD_READ_COIL:
    {
        uint16_t startaddr = 0;
        uint16_t data = 0;
        uint16_t crc = 0;
        memset(SendData->Frame.Buf, 0, sizeof(SendData->Frame.Buf));
        SendData->Frame.Buf[0] = SendData->Data.DeviceID;
        SendData->Frame.Buf[1] = Cmd;
        startaddr = SWAP16(SendData->Data.StartAddr);
        data = SWAP16(SendData->Data.Data);
        memcpy_s(SendData->Frame.Buf + 2, sizeof(uint16_t), &startaddr, sizeof(uint16_t));
        memcpy_s(SendData->Frame.Buf + 4, sizeof(uint16_t), &data, sizeof(uint16_t));
        crc = CalCRC(SendData->Frame.Buf, 6);
        memcpy_s(SendData->Frame.Buf + 6, sizeof(uint16_t), &crc, sizeof(uint16_t));

        //m_Comm_Response_Check[m_Hash_AddressToMonitor[SendData->Data.DeviceID]] = 0;
        emit ReadySendData(reinterpret_cast<const char*>(SendData->Frame.Buf), 8);
    }
        break;

    case TEMP_CONTROLLER_CMD_READ_DISCRETE_INPUTS:
        break;

    case TEMP_CONTROLLER_CMD_READ_HOLDING_REGISTERS:
    {
        uint16_t startaddr = 0;
        uint16_t data = 0;
        uint16_t crc = 0;
        memset(SendData->Frame.Buf, 0, sizeof(SendData->Frame.Buf));
        SendData->Frame.Buf[0] = SendData->Data.DeviceID;
        SendData->Frame.Buf[1] = Cmd;
        startaddr = SWAP16(SendData->Data.StartAddr);
        data = SWAP16(SendData->Data.Data);
        memcpy_s(SendData->Frame.Buf + 2, sizeof(uint16_t), &startaddr, sizeof(uint16_t));
        memcpy_s(SendData->Frame.Buf + 4, sizeof(uint16_t), &data, sizeof(uint16_t));
        crc = CalCRC(SendData->Frame.Buf, 6);
        memcpy_s(SendData->Frame.Buf + 6, sizeof(uint16_t), &crc, sizeof(uint16_t));

        //m_Comm_Response_Check[m_Hash_AddressToMonitor[SendData->Data.DeviceID]] = 0;
        emit ReadySendData(reinterpret_cast<const char*>(SendData->Frame.Buf), 8);
    }
        break;

    case TEMP_CONTROLLER_CMD_READ_INPUT_REGISTER:
    {
        uint16_t startaddr = 0;
        uint16_t data = 0;
        uint16_t crc = 0;
        memset(SendData->Frame.Buf, 0, sizeof(SendData->Frame.Buf));
        SendData->Frame.Buf[0] = SendData->Data.DeviceID;
        SendData->Frame.Buf[1] = Cmd;
        startaddr = SWAP16(SendData->Data.StartAddr);
        data = SWAP16(SendData->Data.Data);
        memcpy_s(SendData->Frame.Buf + 2, sizeof(uint16_t), &startaddr, sizeof(uint16_t));
        memcpy_s(SendData->Frame.Buf + 4, sizeof(uint16_t), &data, sizeof(uint16_t));
        crc = CalCRC(SendData->Frame.Buf, 6);
        memcpy_s(SendData->Frame.Buf + 6, sizeof(uint16_t), &crc, sizeof(uint16_t));

        //m_Comm_Response_Check[m_Hash_AddressToMonitor[SendData->Data.DeviceID]] = 0;
        emit ReadySendData(reinterpret_cast<const char*>(SendData->Frame.Buf), 8);
    }
        break;

    case TEMP_CONTROLLER_CMD_WRITE_SINGLE_COIL:
        break;

    case TEMP_CONTROLLER_CMD_WRITE_SINGLE_REGISTER:
    {
        uint16_t startaddr = 0;
        uint16_t data = 0;
        uint16_t crc = 0;
        memset(SendData->Frame.Buf, 0, sizeof(SendData->Frame.Buf));
        SendData->Frame.Buf[0] = SendData->Data.DeviceID;
        SendData->Frame.Buf[1] = Cmd;
        startaddr = SWAP16(SendData->Data.StartAddr);
        data = SWAP16(SendData->Data.Data);
        memcpy_s(SendData->Frame.Buf + 2, sizeof(uint16_t), &startaddr, sizeof(uint16_t));
        memcpy_s(SendData->Frame.Buf + 4, sizeof(uint16_t), &data, sizeof(uint16_t));
        crc = CalCRC(SendData->Frame.Buf, 6);
        memcpy_s(SendData->Frame.Buf + 6, sizeof(uint16_t), &crc, sizeof(uint16_t));

        //m_Comm_Response_Check[m_Hash_AddressToMonitor[SendData->Data.DeviceID]] = 0;
        emit ReadySendData(reinterpret_cast<const char*>(SendData->Frame.Buf), 8);
    }
        break;

    case TEMP_CONTROLLER_CMD_WRITE_MULTIPLE_COILS:
        break;

    case TEMP_CONTROLLER_CMD_WRITE_MULTIPLE_REGISTERS:
        break;

    default:
        break;

    }
}

uint16_t CThread_Temp_Controller_ProcessCmdbuf::CalCRC(uint8_t* cBuf, uint8_t cLen)
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

void CThread_Temp_Controller_ProcessCmdbuf::SetCmdStatus(quint8 index, quint16 status)
{
    m_Cmd_Status[index] = status;
}
