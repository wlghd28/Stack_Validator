#include "cthread_flow_controller_processcommbuf.h"
#include "cflow_controller.h"

CThread_Flow_Controller_ProcessCommbuf::CThread_Flow_Controller_ProcessCommbuf(CFlow_Controller* parent, CQueue* queue)
{
    m_parent = parent;
    m_Queue = queue;

    Init();
}

CThread_Flow_Controller_ProcessCommbuf::~CThread_Flow_Controller_ProcessCommbuf()
{
}

/*
    override methods
*/
void CThread_Flow_Controller_ProcessCommbuf::run()
{
    ProcessCommbuf();
}

/*
    methods
*/
void CThread_Flow_Controller_ProcessCommbuf::Init()
{
    m_bFlag_Thread_Quit = false;
    m_bCheck_Thread_Quit = false;
}

void CThread_Flow_Controller_ProcessCommbuf::StopThread()
{
    m_bFlag_Thread_Quit = true;
    while(!m_bCheck_Thread_Quit){};  
    quit();
    wait();
}

void CThread_Flow_Controller_ProcessCommbuf::ProcessCommbuf()
{
    quint8 TempBuf[MAXBUFSIZE]; // Temp Data array

    while(!m_bFlag_Thread_Quit)
    {   
        msleep(50);

        uint16_t bufsize = m_Queue->GetDataLength();

        // 버퍼에 데이터가 없는 경우
        if(!bufsize) continue;

        uint8_t start_stop_bit_count = 0;
        uint8_t start_bit_index = 0;
        uint8_t stop_bit_index = 0;
        uint16_t index = 0;
        uint16_t framesize = 0;

        // 버퍼 메모리 초기화
        memset(TempBuf, 0, bufsize * sizeof(uint8_t));
        //memset(TempBuf, 0, MAXBUFSIZE * sizeof(uint8_t));

        // 버퍼 메모리에 메모리 복사 및 Start, Stop 비트 조사
//        index = m_Queue->GetTail();
//        for(int i = 0; i < bufsize; i++)
//        {
//            TempBuf[i] = m_Queue->GetData(index);
//            if(TempBuf[i] == FLOW_CONTROLLER_START_STOP_BIT)
//            {
//                start_stop_bit_count++;
//                start_bit_index = i * (start_stop_bit_count == 1);
//                stop_bit_index = i * (start_stop_bit_count == 2);
//            }
//            index++;
//            index &= (MAXBUFSIZE - 1);
//        }

//        // start_stop_bit_count < 2 이면 무시
//        if(start_stop_bit_count < 2) continue;

        index = m_Queue->GetTail();
        for(int i = 0; i < bufsize; i++)
        {
            TempBuf[i] = m_Queue->GetData(index++);
            if(TempBuf[i] == FLOW_CONTROLLER_START_STOP_BIT)
            {
                start_stop_bit_count++;
                start_bit_index = i;
                break;
            }

            m_Queue->Delete(1);
            index &= (MAXBUFSIZE - 1);
        }

        // start_stop_bit_count < 1 이면 무시
        if(start_stop_bit_count < 1) continue;

        for(int i = start_bit_index + 1; i < bufsize; i++)
        {
            TempBuf[i] = m_Queue->GetData(index++);
            if(TempBuf[i] == FLOW_CONTROLLER_START_STOP_BIT)
            {
                start_stop_bit_count++;
                stop_bit_index = i;
                break;
            }

            index &= (MAXBUFSIZE - 1);
        }

        // start_stop_bit_count < 2 이면 무시
        if(start_stop_bit_count < 2) continue;

        // Frame 구조체에 메모리 복사
        framesize = stop_bit_index - start_bit_index + 1;
        index = start_bit_index;

        // Start
        m_parent->m_STC_Comm_MISO.Start = TempBuf[start_bit_index];

        // Adr
        index++;
        if(TempBuf[index] == 0x7D) m_parent->m_STC_Comm_MISO.Adr = (TempBuf[++index] ^ HDLC_BYTE_STUFFING);
        else m_parent->m_STC_Comm_MISO.Adr = TempBuf[index];

        // Cmd
        index++;
        if(TempBuf[index] == 0x7D) m_parent->m_STC_Comm_MISO.Cmd = (TempBuf[++index] ^ HDLC_BYTE_STUFFING);
        else m_parent->m_STC_Comm_MISO.Cmd = TempBuf[index];

        // State
        index++;
        if(TempBuf[index] == 0x7D) m_parent->m_STC_Comm_MISO.State.Val = (TempBuf[++index] ^ HDLC_BYTE_STUFFING);
        else m_parent->m_STC_Comm_MISO.State.Val = TempBuf[index];

        // Len
        index++;
        if(TempBuf[index] == 0x7D) m_parent->m_STC_Comm_MISO.Len = (TempBuf[++index] ^ HDLC_BYTE_STUFFING);
        else m_parent->m_STC_Comm_MISO.Len = TempBuf[index];

        // Data
        memset(m_parent->m_STC_Comm_MISO.Data, 0, sizeof(m_parent->m_STC_Comm_MISO.Data));
        for(uint16_t i = 0; i < m_parent->m_STC_Comm_MISO.Len; i++)
        {
            index++;
            if(TempBuf[index] == 0x7D) m_parent->m_STC_Comm_MISO.Data[i] = (TempBuf[++index] ^ HDLC_BYTE_STUFFING);
            else m_parent->m_STC_Comm_MISO.Data[i] = TempBuf[index];
        }

        // Chk
        index++;
        if(TempBuf[index] == 0x7D) m_parent->m_STC_Comm_MISO.Chk = (TempBuf[++index] ^ HDLC_BYTE_STUFFING);
        else m_parent->m_STC_Comm_MISO.Chk = TempBuf[index];

        // Stop
        m_parent->m_STC_Comm_MISO.Stop = TempBuf[stop_bit_index];

        // Crc Check
        uint64_t sum = m_parent->m_STC_Comm_MISO.Adr + m_parent->m_STC_Comm_MISO.Cmd + m_parent->m_STC_Comm_MISO.Len;
        if(m_parent->m_STC_Comm_MISO.Chk == CalCheckSum(m_parent->m_STC_Comm_MISO.Data, m_parent->m_STC_Comm_MISO.Len, sum))
        {
            uint8_t monitor_num = m_parent->m_Hash_AddressToMonitor[m_parent->m_STC_Comm_MISO.Adr];

            // 데이터 복사
            memcpy_s
            (
                &(m_parent->m_STC_Comm_RecvData[monitor_num].Frame),
                sizeof(STC_FLOW_CONTROLLER_MISO),
                &m_parent->m_STC_Comm_MISO,
                sizeof(STC_FLOW_CONTROLLER_MISO)
            );

            // 데이터 파싱
            switch(m_parent->m_STC_Comm_MISO.Cmd)
            {
            case 0x00:   // Get/Set Point
            {
                if(m_parent->m_STC_Comm_MISO.Len > 0x00)
                {
                    ByteSwapping(m_parent->m_STC_Comm_MISO.Data, m_parent->m_STC_Comm_MISO.Len);
                    memcpy_s(&(m_parent->m_STC_Comm_RecvData[monitor_num].Data.Point), sizeof(float), m_parent->m_STC_Comm_MISO.Data, sizeof(float));
                }
                m_parent->m_Comm_Response_Check[monitor_num] = 1;
            }
                break;

            case 0x08:   // Read Measured Value
            {
                ByteSwapping(m_parent->m_STC_Comm_MISO.Data, m_parent->m_STC_Comm_MISO.Len);
                memcpy_s(&(m_parent->m_STC_Comm_RecvData[monitor_num].Data.Meas), sizeof(float), m_parent->m_STC_Comm_MISO.Data, sizeof(float));
            }
                break;

            default:
                break;
            }

        }

        m_Queue->Delete((uint16_t)framesize);
    }

    m_bCheck_Thread_Quit = true;
}

void CThread_Flow_Controller_ProcessCommbuf::ByteSwapping(uint8_t* buf, uint64_t len)
{
    uint8_t* temp = new uint8_t[len];

    for(uint64_t i = 0; i < len; i++)
    {
        temp[i] = buf[len - 1 - i];
    }

    memcpy_s(buf, sizeof(uint8_t) * len, temp, sizeof(uint8_t) * len);

    if(temp) delete[] temp;
}

uint8_t CThread_Flow_Controller_ProcessCommbuf::CalCheckSum(uint8_t* buf, uint8_t len, uint64_t sum)
{
    uint64_t l_sum = sum;
    uint8_t mem[8] = { 0, };

    for(uint16_t i = 0; i < len; i++)
    {
        l_sum += buf[i];
    }

    memcpy_s(mem, sizeof(mem), &l_sum, sizeof(l_sum));
    return ~mem[0];
}
