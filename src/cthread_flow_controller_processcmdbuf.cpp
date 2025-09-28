#include "cthread_flow_controller_processcmdbuf.h"
#include "Stack_Validator.h"

CThread_Flow_Controller_ProcessCmdbuf::CThread_Flow_Controller_ProcessCmdbuf(CFlow_Controller* parent)
{
    m_parent = parent;

    Init();
}

CThread_Flow_Controller_ProcessCmdbuf::~CThread_Flow_Controller_ProcessCmdbuf()
{
}

/*
    override methods
*/
void CThread_Flow_Controller_ProcessCmdbuf::run()
{
    ProcessCmdStatus();
}

/*
    methods
*/
void CThread_Flow_Controller_ProcessCmdbuf::Init()
{
    m_bFlag_Thread_Quit = false;
    m_bCheck_Thread_Quit = false;
    for(int i = 0; i < LOADER_CHANNEL; i++)
    {
        m_Cmd_Status[i] = static_cast<quint16>(CMD_FLOWCONTROLLER_REQUESTDATA::NONE);
    }
}

void CThread_Flow_Controller_ProcessCmdbuf::StopThread()
{
    m_bFlag_Thread_Quit = true;
    while(!m_bCheck_Thread_Quit){};
    quit();
    wait();
}

void CThread_Flow_Controller_ProcessCmdbuf::ProcessCmdStatus()
{
    quint8 monitor_index = 0;

    while(!m_bFlag_Thread_Quit)
    {
        switch(m_parent->m_parent->m_Active_Mode[monitor_index])
        {
        case static_cast<quint16>(ACTIVE_MODE::METHANOL):
            switch(m_Cmd_Status[monitor_index])
            {
            case static_cast<quint16>(CMD_FLOWCONTROLLER_REQUESTDATA::RESET_CATHODE):
#ifndef FLOW_PIDCONTROL
                m_parent->GetFlowControllerPoint(monitor_index);
#else
#endif
                ProcessCmdbuf(FLOW_CONTROLLER_CMD_RESET, &(m_parent->m_STC_Comm_SendData[monitor_index]));
                break;

            case static_cast<quint16>(CMD_FLOWCONTROLLER_REQUESTDATA::RESET_BROADCAST):
#ifndef FLOW_PIDCONTROL
                m_parent->GetFlowControllerPoint(monitor_index);
#else

#endif
                ProcessCmdbuf(FLOW_CONTROLLER_CMD_RESET_BROADCAST, &(m_parent->m_STC_Comm_SendData[monitor_index]));
                break;

            case static_cast<quint16>(CMD_FLOWCONTROLLER_REQUESTDATA::SETPOINT_CATHODE):
                if(m_parent->m_Comm_Response_Check[monitor_index])
                {
#ifndef FLOW_PIDCONTROL
                    m_parent->GetFlowControllerPoint(monitor_index);
                    ProcessCmdbuf(FLOW_CONTROLLER_CMD_GET_POINT, &(m_parent->m_STC_Comm_SendData[monitor_index]));
#else
#endif
                }
                else
                {
                    ProcessCmdbuf(FLOW_CONTROLLER_CMD_SET_POINT, &(m_parent->m_STC_Comm_SendData[monitor_index]));
                }
                break;

            case static_cast<quint16>(CMD_FLOWCONTROLLER_REQUESTDATA::GETPOINT_CATHODE):
                if(m_parent->m_Comm_Response_Check[monitor_index])
                {
                    m_parent->GetFlowControllerMeas(monitor_index);
                    ProcessCmdbuf(FLOW_CONTROLLER_CMD_GET_MEAS, &(m_parent->m_STC_Comm_SendData[monitor_index]));
                }
                else
                {
                    ProcessCmdbuf(FLOW_CONTROLLER_CMD_GET_POINT, &(m_parent->m_STC_Comm_SendData[monitor_index]));
                }
                break;

            case static_cast<quint16>(CMD_FLOWCONTROLLER_REQUESTDATA::GETMEAS_CATHODE):
#ifndef FLOW_PIDCONTROL
                ProcessCmdbuf(FLOW_CONTROLLER_CMD_GET_MEAS, &(m_parent->m_STC_Comm_SendData[monitor_index]));
                m_parent->ErrHandleFlowController(monitor_index, static_cast<quint16>(FLOW_CONTROLLER::CATHODE));
                if(m_parent->m_parent->m_ptrMonitor[monitor_index] == nullptr)
                    m_Cmd_Status[monitor_index] = static_cast<quint16>(CMD_FLOWCONTROLLER_REQUESTDATA::NONE);
#else
#endif
                break;

            default:
                break;
            }
            break;

        case static_cast<quint16>(ACTIVE_MODE::HYDROGEN):
            break;

        default:
            break;
        }

        if(++monitor_index >= LOADER_CHANNEL)
            monitor_index = 0;

        msleep(FLOWCONTROLLER_REQUEST_TIMERCLOCK);
    }

    m_bCheck_Thread_Quit = true;
}

void CThread_Flow_Controller_ProcessCmdbuf::ProcessCmdbuf(quint16 Cmd, STC_COMM_FLOW_CONTROLLER_SEND* SendData)
{
    switch(Cmd)
    {
    case FLOW_CONTROLLER_CMD_RESET:
    {
        uint8_t frame[FRAME_MAXBUFSIZE] = { 0, };
        uint8_t index_frame = 0;
        uint64_t sum = SendData->Frame.Adr + SendData->Frame.Cmd + SendData->Frame.Len;

        memset(&SendData->Frame, 0, sizeof(SendData->Frame));
        SendData->Frame.Start = FLOW_CONTROLLER_START_STOP_BIT;
        SendData->Frame.Adr = SendData->Data.Address;
        SendData->Frame.Cmd = 0xD3;
        SendData->Frame.Len = 0x00;
        SendData->Frame.Data = new uint8_t[SendData->Frame.Len];
        memset(SendData->Frame.Data, 0, sizeof(uint8_t) * (SendData->Frame.Len));
        SendData->Frame.Chk = CalCheckSum(SendData->Frame.Data, SendData->Frame.Len, sum);
        SendData->Frame.Stop = FLOW_CONTROLLER_START_STOP_BIT;
        ByteStuffing_Frame_MOSI(SendData, frame, &index_frame);

        //m_Comm_Response_Check[m_Hash_AddressToMonitor[SendData->Data.Address]] = 0;
        emit ReadySendData(reinterpret_cast<const char*>(frame), ++index_frame);
    }
        break;

    case FLOW_CONTROLLER_CMD_RESET_BROADCAST:
    {
        uint8_t frame[FRAME_MAXBUFSIZE] = { 0, };
        uint8_t index_frame = 0;
        uint64_t sum = SendData->Frame.Adr + SendData->Frame.Cmd + SendData->Frame.Len;

        memset(&SendData->Frame, 0, sizeof(SendData->Frame));
        SendData->Frame.Start = FLOW_CONTROLLER_START_STOP_BIT;
        SendData->Frame.Adr = 0xFF;
        SendData->Frame.Cmd = 0xD3;
        SendData->Frame.Len = 0x00;
        SendData->Frame.Data = new uint8_t[SendData->Frame.Len];
        memset(SendData->Frame.Data, 0, sizeof(uint8_t) * (SendData->Frame.Len));
        SendData->Frame.Chk = CalCheckSum(SendData->Frame.Data, SendData->Frame.Len, sum);
        SendData->Frame.Stop = FLOW_CONTROLLER_START_STOP_BIT;
        ByteStuffing_Frame_MOSI(SendData, frame, &index_frame);

        //m_Comm_Response_Check[m_Hash_AddressToMonitor[SendData->Data.Address]] = 0;
        emit ReadySendData(reinterpret_cast<const char*>(frame), ++index_frame);
    }
        break;

    case FLOW_CONTROLLER_CMD_GET_ADDR:
    {
    }
        break;

    case FLOW_CONTROLLER_CMD_SET_ADDR:
    {
        uint8_t frame[FRAME_MAXBUFSIZE] = { 0, };
        uint8_t index_frame = 0;
        uint64_t sum = SendData->Frame.Adr + SendData->Frame.Cmd + SendData->Frame.Len;

        memset(&SendData->Frame, 0, sizeof(SendData->Frame));
        SendData->Frame.Start = FLOW_CONTROLLER_START_STOP_BIT;
        //SendData->Frame.Adr = SendData->Frame.Adr;
        SendData->Frame.Cmd = 0x90;
        SendData->Frame.Len = 0x01;
        SendData->Frame.Data = new uint8_t[SendData->Frame.Len];
        memset(SendData->Frame.Data, 0, sizeof(uint8_t) * (SendData->Frame.Len));
        SendData->Frame.Data[0] = SendData->Data.Address;
        SendData->Frame.Chk = CalCheckSum(SendData->Frame.Data, SendData->Frame.Len, sum);
        SendData->Frame.Stop = FLOW_CONTROLLER_START_STOP_BIT;
        ByteStuffing_Frame_MOSI(SendData, frame, &index_frame);

        //m_Comm_Response_Check[m_Hash_AddressToMonitor[SendData->Data.Address]] = 0;
        emit ReadySendData(reinterpret_cast<const char*>(frame), ++index_frame);
    }
        break;

    case FLOW_CONTROLLER_CMD_GET_MEAS:
    {
        uint8_t frame[FRAME_MAXBUFSIZE] = { 0, };
        uint8_t index_frame = 0;
        uint64_t sum = SendData->Frame.Adr + SendData->Frame.Cmd + SendData->Frame.Len;

        memset(&SendData->Frame, 0, sizeof(SendData->Frame));
        SendData->Frame.Start = FLOW_CONTROLLER_START_STOP_BIT;
        SendData->Frame.Adr = SendData->Data.Address;
        SendData->Frame.Cmd = 0x08;
        SendData->Frame.Len = 0x01;
        SendData->Frame.Data = new uint8_t[SendData->Frame.Len];
        memset(SendData->Frame.Data, 0, sizeof(uint8_t) * (SendData->Frame.Len));
        SendData->Frame.Data[0] = 0x01;
        SendData->Frame.Chk = CalCheckSum(SendData->Frame.Data, SendData->Frame.Len, sum);
        SendData->Frame.Stop = FLOW_CONTROLLER_START_STOP_BIT;
        ByteStuffing_Frame_MOSI(SendData, frame, &index_frame);

        //m_Comm_Response_Check[m_Hash_AddressToMonitor[SendData->Data.Address]] = 0;
        emit ReadySendData(reinterpret_cast<const char*>(frame), ++index_frame);
    }
        break;

    case FLOW_CONTROLLER_CMD_GET_POINT:
    {
        uint8_t frame[FRAME_MAXBUFSIZE] = { 0, };
        uint8_t index_frame = 0;
        uint64_t sum = SendData->Frame.Adr + SendData->Frame.Cmd + SendData->Frame.Len;

        memset(&SendData->Frame, 0, sizeof(SendData->Frame));
        SendData->Frame.Start = FLOW_CONTROLLER_START_STOP_BIT;
        SendData->Frame.Adr = SendData->Data.Address;
        SendData->Frame.Cmd = 0x00;
        SendData->Frame.Len = 0x01;
        SendData->Frame.Data = new uint8_t[SendData->Frame.Len];
        memset(SendData->Frame.Data, 0, sizeof(uint8_t) * (SendData->Frame.Len));
        SendData->Frame.Data[0] = 0x01;
        SendData->Frame.Chk = CalCheckSum(SendData->Frame.Data, SendData->Frame.Len, sum);
        SendData->Frame.Stop = FLOW_CONTROLLER_START_STOP_BIT;
        ByteStuffing_Frame_MOSI(SendData, frame, &index_frame);

        //m_Comm_Response_Check[m_Hash_AddressToMonitor[SendData->Data.Address]] = 0;
        emit ReadySendData(reinterpret_cast<const char*>(frame), ++index_frame);
    }
        break;

    case FLOW_CONTROLLER_CMD_SET_POINT:
    {
        uint8_t frame[FRAME_MAXBUFSIZE] = { 0, };
        uint8_t index_frame = 0;
        uint64_t sum = SendData->Frame.Adr + SendData->Frame.Cmd + SendData->Frame.Len;

        memset(&SendData->Frame, 0, sizeof(SendData->Frame));
        SendData->Frame.Start = FLOW_CONTROLLER_START_STOP_BIT;
        SendData->Frame.Adr = SendData->Data.Address;
        SendData->Frame.Cmd = 0x00;
        SendData->Frame.Len = 0x05;
        SendData->Frame.Data = new uint8_t[SendData->Frame.Len];
        memset(SendData->Frame.Data, 0, sizeof(uint8_t) * (SendData->Frame.Len));
        SendData->Frame.Data[0] = 0x01;
        memcpy_s(SendData->Frame.Data + 1, sizeof(float), &(SendData->Data.Point), sizeof(float));
        ByteSwapping(SendData->Frame.Data + 1, SendData->Frame.Len - 0x01);
        SendData->Frame.Chk = CalCheckSum(SendData->Frame.Data, SendData->Frame.Len, sum);
        SendData->Frame.Stop = FLOW_CONTROLLER_START_STOP_BIT;
        ByteStuffing_Frame_MOSI(SendData, frame, &index_frame);

        //m_Comm_Response_Check[m_Hash_AddressToMonitor[SendData->Data.Address]] = 0;
        emit ReadySendData(reinterpret_cast<const char*>(frame), ++index_frame);
    }
        break;

    default:
        break;
    }

    if(SendData->Frame.Data)
    {
        delete[] SendData->Frame.Data;
        SendData->Frame.Data = nullptr;
    }
}

void CThread_Flow_Controller_ProcessCmdbuf::ProcessCmdbuf_OtherThread(quint16 Cmd, STC_COMM_FLOW_CONTROLLER_SEND* SendData)
{
    switch(Cmd)
    {
    case FLOW_CONTROLLER_CMD_RESET:
    {
        uint8_t frame[FRAME_MAXBUFSIZE] = { 0, };
        uint8_t index_frame = 0;
        uint64_t sum = SendData->Frame.Adr + SendData->Frame.Cmd + SendData->Frame.Len;

        memset(&SendData->Frame, 0, sizeof(SendData->Frame));
        SendData->Frame.Start = FLOW_CONTROLLER_START_STOP_BIT;
        SendData->Frame.Adr = SendData->Data.Address;
        SendData->Frame.Cmd = 0xD3;
        SendData->Frame.Len = 0x00;
        SendData->Frame.Data = new uint8_t[SendData->Frame.Len];
        memset(SendData->Frame.Data, 0, sizeof(uint8_t) * (SendData->Frame.Len));
        SendData->Frame.Chk = CalCheckSum(SendData->Frame.Data, SendData->Frame.Len, sum);
        SendData->Frame.Stop = FLOW_CONTROLLER_START_STOP_BIT;
        ByteStuffing_Frame_MOSI(SendData, frame, &index_frame);

        //m_Comm_Response_Check[m_Hash_AddressToMonitor[SendData->Data.Address]] = 0;
        emit ReadySendData_OtherThread(reinterpret_cast<const char*>(frame), ++index_frame);
    }
        break;

    case FLOW_CONTROLLER_CMD_RESET_BROADCAST:
    {
        uint8_t frame[FRAME_MAXBUFSIZE] = { 0, };
        uint8_t index_frame = 0;
        uint64_t sum = SendData->Frame.Adr + SendData->Frame.Cmd + SendData->Frame.Len;

        memset(&SendData->Frame, 0, sizeof(SendData->Frame));
        SendData->Frame.Start = FLOW_CONTROLLER_START_STOP_BIT;
        SendData->Frame.Adr = 0xFF;
        SendData->Frame.Cmd = 0xD3;
        SendData->Frame.Len = 0x00;
        SendData->Frame.Data = new uint8_t[SendData->Frame.Len];
        memset(SendData->Frame.Data, 0, sizeof(uint8_t) * (SendData->Frame.Len));
        SendData->Frame.Chk = CalCheckSum(SendData->Frame.Data, SendData->Frame.Len, sum);
        SendData->Frame.Stop = FLOW_CONTROLLER_START_STOP_BIT;
        ByteStuffing_Frame_MOSI(SendData, frame, &index_frame);

        //m_Comm_Response_Check[m_Hash_AddressToMonitor[SendData->Data.Address]] = 0;
        emit ReadySendData_OtherThread(reinterpret_cast<const char*>(frame), ++index_frame);
    }
        break;

    case FLOW_CONTROLLER_CMD_GET_ADDR:
    {
    }
        break;

    case FLOW_CONTROLLER_CMD_SET_ADDR:
    {
        uint8_t frame[FRAME_MAXBUFSIZE] = { 0, };
        uint8_t index_frame = 0;
        uint64_t sum = SendData->Frame.Adr + SendData->Frame.Cmd + SendData->Frame.Len;

        memset(&SendData->Frame, 0, sizeof(SendData->Frame));
        SendData->Frame.Start = FLOW_CONTROLLER_START_STOP_BIT;
        //SendData->Frame.Adr = SendData->Frame.Adr;
        SendData->Frame.Cmd = 0x90;
        SendData->Frame.Len = 0x01;
        SendData->Frame.Data = new uint8_t[SendData->Frame.Len];
        memset(SendData->Frame.Data, 0, sizeof(uint8_t) * (SendData->Frame.Len));
        SendData->Frame.Data[0] = SendData->Data.Address;
        SendData->Frame.Chk = CalCheckSum(SendData->Frame.Data, SendData->Frame.Len, sum);
        SendData->Frame.Stop = FLOW_CONTROLLER_START_STOP_BIT;
        ByteStuffing_Frame_MOSI(SendData, frame, &index_frame);

        //m_Comm_Response_Check[m_Hash_AddressToMonitor[SendData->Data.Address]] = 0;
        emit ReadySendData_OtherThread(reinterpret_cast<const char*>(frame), ++index_frame);
    }
        break;

    case FLOW_CONTROLLER_CMD_GET_MEAS:
    {
        uint8_t frame[FRAME_MAXBUFSIZE] = { 0, };
        uint8_t index_frame = 0;
        uint64_t sum = SendData->Frame.Adr + SendData->Frame.Cmd + SendData->Frame.Len;

        memset(&SendData->Frame, 0, sizeof(SendData->Frame));
        SendData->Frame.Start = FLOW_CONTROLLER_START_STOP_BIT;
        SendData->Frame.Adr = SendData->Data.Address;
        SendData->Frame.Cmd = 0x08;
        SendData->Frame.Len = 0x01;
        SendData->Frame.Data = new uint8_t[SendData->Frame.Len];
        memset(SendData->Frame.Data, 0, sizeof(uint8_t) * (SendData->Frame.Len));
        SendData->Frame.Data[0] = 0x01;
        SendData->Frame.Chk = CalCheckSum(SendData->Frame.Data, SendData->Frame.Len, sum);
        SendData->Frame.Stop = FLOW_CONTROLLER_START_STOP_BIT;
        ByteStuffing_Frame_MOSI(SendData, frame, &index_frame);

        //m_Comm_Response_Check[m_Hash_AddressToMonitor[SendData->Data.Address]] = 0;
        emit ReadySendData_OtherThread(reinterpret_cast<const char*>(frame), ++index_frame);
    }
        break;

    case FLOW_CONTROLLER_CMD_GET_POINT:
    {
        uint8_t frame[FRAME_MAXBUFSIZE] = { 0, };
        uint8_t index_frame = 0;
        uint64_t sum = SendData->Frame.Adr + SendData->Frame.Cmd + SendData->Frame.Len;

        memset(&SendData->Frame, 0, sizeof(SendData->Frame));
        SendData->Frame.Start = FLOW_CONTROLLER_START_STOP_BIT;
        SendData->Frame.Adr = SendData->Data.Address;
        SendData->Frame.Cmd = 0x00;
        SendData->Frame.Len = 0x01;
        SendData->Frame.Data = new uint8_t[SendData->Frame.Len];
        memset(SendData->Frame.Data, 0, sizeof(uint8_t) * (SendData->Frame.Len));
        SendData->Frame.Data[0] = 0x01;
        SendData->Frame.Chk = CalCheckSum(SendData->Frame.Data, SendData->Frame.Len, sum);
        SendData->Frame.Stop = FLOW_CONTROLLER_START_STOP_BIT;
        ByteStuffing_Frame_MOSI(SendData, frame, &index_frame);

        //m_Comm_Response_Check[m_Hash_AddressToMonitor[SendData->Data.Address]] = 0;
        emit ReadySendData_OtherThread(reinterpret_cast<const char*>(frame), ++index_frame);
    }
        break;

    case FLOW_CONTROLLER_CMD_SET_POINT:
    {
        uint8_t frame[FRAME_MAXBUFSIZE] = { 0, };
        uint8_t index_frame = 0;
        uint64_t sum = SendData->Frame.Adr + SendData->Frame.Cmd + SendData->Frame.Len;

        memset(&SendData->Frame, 0, sizeof(SendData->Frame));
        SendData->Frame.Start = FLOW_CONTROLLER_START_STOP_BIT;
        SendData->Frame.Adr = SendData->Data.Address;
        SendData->Frame.Cmd = 0x00;
        SendData->Frame.Len = 0x05;
        SendData->Frame.Data = new uint8_t[SendData->Frame.Len];
        memset(SendData->Frame.Data, 0, sizeof(uint8_t) * (SendData->Frame.Len));
        SendData->Frame.Data[0] = 0x01;
        memcpy_s(SendData->Frame.Data + 1, sizeof(float), &(SendData->Data.Point), sizeof(float));
        ByteSwapping(SendData->Frame.Data + 1, SendData->Frame.Len - 0x01);
        SendData->Frame.Chk = CalCheckSum(SendData->Frame.Data, SendData->Frame.Len, sum);
        SendData->Frame.Stop = FLOW_CONTROLLER_START_STOP_BIT;
        ByteStuffing_Frame_MOSI(SendData, frame, &index_frame);

        //m_Comm_Response_Check[m_Hash_AddressToMonitor[SendData->Data.Address]] = 0;
        emit ReadySendData_OtherThread(reinterpret_cast<const char*>(frame), ++index_frame);
    }
        break;

    default:
        break;
    }

    if(SendData->Frame.Data)
    {
        delete[] SendData->Frame.Data;
        SendData->Frame.Data = nullptr;
    }
}

void CThread_Flow_Controller_ProcessCmdbuf::ByteStuffing_Frame_MOSI(STC_COMM_FLOW_CONTROLLER_SEND* SendData, uint8_t* frame, uint8_t* index_frame)
{
    // Start
    frame[(*index_frame)++] = SendData->Frame.Start;

    // Adr
    if(SendData->Frame.Adr == 0x7E || SendData->Frame.Adr == 0x7D || SendData->Frame.Adr == 0x11 || SendData->Frame.Adr == 0x13)
    {
        frame[(*index_frame)++] = 0x7D;
        frame[(*index_frame)++] = (SendData->Frame.Adr ^ HDLC_BYTE_STUFFING);
    }
    else frame[(*index_frame)++] = SendData->Frame.Adr;

    // Cmd
    if(SendData->Frame.Cmd == 0x7E || SendData->Frame.Cmd == 0x7D || SendData->Frame.Cmd == 0x11 || SendData->Frame.Cmd == 0x13)
    {
        frame[(*index_frame)++] = 0x7D;
        frame[(*index_frame)++] = (SendData->Frame.Cmd ^ HDLC_BYTE_STUFFING);
    }
    else frame[(*index_frame)++] = SendData->Frame.Cmd;

    // Len
    if(SendData->Frame.Len == 0x7E || SendData->Frame.Len == 0x7D || SendData->Frame.Len == 0x11 || SendData->Frame.Len == 0x13)
    {
        frame[(*index_frame)++] = 0x7D;
        frame[(*index_frame)++] = (SendData->Frame.Len ^ HDLC_BYTE_STUFFING);
    }
    else frame[(*index_frame)++] = SendData->Frame.Len;

    // Data
    for(uint16_t i = 0; i < SendData->Frame.Len; i++)
    {
        if(SendData->Frame.Data[i] == 0x7E || SendData->Frame.Data[i] == 0x7D || SendData->Frame.Data[i] == 0x11 || SendData->Frame.Data[i] == 0x13)
        {
            frame[(*index_frame)++] = 0x7D;
            frame[(*index_frame)++] = (SendData->Frame.Data[i] ^ HDLC_BYTE_STUFFING);
        }
        else frame[(*index_frame)++] = SendData->Frame.Data[i];
    }

    // Chk
    if(SendData->Frame.Chk == 0x7E || SendData->Frame.Chk == 0x7D || SendData->Frame.Chk == 0x11 || SendData->Frame.Chk == 0x13)
    {
        frame[(*index_frame)++] = 0x7D;
        frame[(*index_frame)++] = (SendData->Frame.Chk ^ HDLC_BYTE_STUFFING);
    }
    else frame[(*index_frame)++] = SendData->Frame.Chk;

    // Stop
    frame[(*index_frame)] = SendData->Frame.Stop;
}

void CThread_Flow_Controller_ProcessCmdbuf::ByteSwapping(uint8_t* buf, uint64_t len)
{
    uint8_t* temp = new uint8_t[len];

    for(uint64_t i = 0; i < len; i++)
    {
        temp[i] = buf[len - 1 - i];
    }

    memcpy_s(buf, sizeof(uint8_t) * len, temp, sizeof(uint8_t) * len);

    if(temp) delete[] temp;
}

uint8_t CThread_Flow_Controller_ProcessCmdbuf::CalCheckSum(uint8_t* buf, uint8_t len, uint64_t sum)
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

void CThread_Flow_Controller_ProcessCmdbuf::SetCmdStatus(quint8 index, quint16 status)
{
    m_Cmd_Status[index] = status;
}
