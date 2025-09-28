#include "cthread_loader_processcmdbuf.h"
#include "Stack_Validator.h"

CThread_Loader_ProcessCmdbuf::CThread_Loader_ProcessCmdbuf(CLoader* parent)
{
    m_parent = parent;

    Init();
}

CThread_Loader_ProcessCmdbuf::~CThread_Loader_ProcessCmdbuf()
{
}

/*
    override methods
*/
void CThread_Loader_ProcessCmdbuf::run()
{
    ProcessCmdStatus();
}


/*
    methods
*/
void CThread_Loader_ProcessCmdbuf::Init()
{
    m_bFlag_Thread_Quit = false;
    m_bCheck_Thread_Quit = false;
    for(int i = 0; i < LOADER_CHANNEL; i++)
    {
        m_Cmd_Status[i] = static_cast<quint16>(CMD_FLOWCONTROLLER_REQUESTDATA::NONE);
    }
}

void CThread_Loader_ProcessCmdbuf::StopThread()
{
    m_bFlag_Thread_Quit = true;
    while(!m_bCheck_Thread_Quit){};
    quit();
    wait();
}

void CThread_Loader_ProcessCmdbuf::ProcessCmdStatus()
{
    quint8 monitor_index = 0;

    while(!m_bFlag_Thread_Quit)
    {
        switch(m_Cmd_Status[monitor_index])
        {
        case static_cast<quint16>(CMD_LOADER_REQUESTDATA::SEQUENCE_INIT1):
            if(m_parent->m_Comm_Response_Check[m_parent->m_parent->m_Loader_Channel[monitor_index]])
                m_Cmd_Status[monitor_index] = static_cast<quint16>(CMD_LOADER_REQUESTDATA::SEQUENCE_INIT2);
            ProcessCmdbuf(LOADER_CMD_OFF, m_parent->m_STC_Comm_SendData + m_parent->m_parent->m_Loader_Channel[monitor_index]);
            break;
        case static_cast<quint16>(CMD_LOADER_REQUESTDATA::SEQUENCE_INIT2):
            if(m_parent->m_Comm_Response_Check[m_parent->m_parent->m_Loader_Channel[monitor_index]])
                m_Cmd_Status[monitor_index] = static_cast<quint16>(CMD_LOADER_REQUESTDATA::SEQUENCE_INIT3);
            ProcessCmdbuf(LOADER_CMD_SET_RANGE_CURR, m_parent->m_STC_Comm_SendData + m_parent->m_parent->m_Loader_Channel[monitor_index]);
            break;
        case static_cast<quint16>(CMD_LOADER_REQUESTDATA::SEQUENCE_INIT3):
            if(m_parent->m_Comm_Response_Check[m_parent->m_parent->m_Loader_Channel[monitor_index]])
                m_Cmd_Status[monitor_index] = static_cast<quint16>(CMD_LOADER_REQUESTDATA::GETALL);
            ProcessCmdbuf(LOADER_CMD_SET_RANGE_VOLT, m_parent->m_STC_Comm_SendData + m_parent->m_parent->m_Loader_Channel[monitor_index]);
            break;
        case static_cast<quint16>(CMD_LOADER_REQUESTDATA::ON):
            if(m_parent->m_Comm_Response_Check[m_parent->m_parent->m_Loader_Channel[monitor_index]])
                m_Cmd_Status[monitor_index] = static_cast<quint16>(CMD_LOADER_REQUESTDATA::GETALL);
            ProcessCmdbuf(LOADER_CMD_ON, m_parent->m_STC_Comm_SendData + m_parent->m_parent->m_Loader_Channel[monitor_index]);
            break;
        case static_cast<quint16>(CMD_LOADER_REQUESTDATA::OFF):
            if(m_parent->m_Comm_Response_Check[m_parent->m_parent->m_Loader_Channel[monitor_index]])
                m_Cmd_Status[monitor_index] = static_cast<quint16>(CMD_LOADER_REQUESTDATA::GETALL);
            ProcessCmdbuf(LOADER_CMD_OFF, m_parent->m_STC_Comm_SendData + m_parent->m_parent->m_Loader_Channel[monitor_index]);
            break;
        case static_cast<quint16>(CMD_LOADER_REQUESTDATA::SETMODE_CC):
            m_parent->m_STC_Comm_SendData[m_parent->m_parent->m_Loader_Channel[monitor_index]].Mode = "CC";
            if(m_parent->m_Comm_Response_Check[m_parent->m_parent->m_Loader_Channel[monitor_index]])
                m_Cmd_Status[monitor_index] = static_cast<quint16>(CMD_LOADER_REQUESTDATA::SEQUENCE_CC);
            ProcessCmdbuf(LOADER_CMD_SET_MODE, m_parent->m_STC_Comm_SendData + m_parent->m_parent->m_Loader_Channel[monitor_index]);
            break;
        case static_cast<quint16>(CMD_LOADER_REQUESTDATA::SETMODE_CV):
            m_parent->m_STC_Comm_SendData[m_parent->m_parent->m_Loader_Channel[monitor_index]].Mode = "CV";
            if(m_parent->m_Comm_Response_Check[m_parent->m_parent->m_Loader_Channel[monitor_index]])
                m_Cmd_Status[monitor_index] = static_cast<quint16>(CMD_LOADER_REQUESTDATA::SEQUENCE_CV);
            ProcessCmdbuf(LOADER_CMD_SET_MODE, m_parent->m_STC_Comm_SendData + m_parent->m_parent->m_Loader_Channel[monitor_index]);
            break;
        case static_cast<quint16>(CMD_LOADER_REQUESTDATA::SETVALUE_CC):
            if(m_parent->m_Comm_Response_Check[m_parent->m_parent->m_Loader_Channel[monitor_index]])
                m_Cmd_Status[monitor_index] = static_cast<quint16>(CMD_LOADER_REQUESTDATA::ON);
            ProcessCmdbuf(LOADER_CMD_SET_CURR, m_parent->m_STC_Comm_SendData + m_parent->m_parent->m_Loader_Channel[monitor_index]);
            break;
        case static_cast<quint16>(CMD_LOADER_REQUESTDATA::SETVALUE_CV):
            if(m_parent->m_Comm_Response_Check[m_parent->m_parent->m_Loader_Channel[monitor_index]])
                m_Cmd_Status[monitor_index] = static_cast<quint16>(CMD_LOADER_REQUESTDATA::ON);
            ProcessCmdbuf(LOADER_CMD_SET_VOLT, m_parent->m_STC_Comm_SendData + m_parent->m_parent->m_Loader_Channel[monitor_index]);
            break;
        case static_cast<quint16>(CMD_LOADER_REQUESTDATA::SEQUENCE_CC):
            if(m_parent->m_Comm_Response_Check[m_parent->m_parent->m_Loader_Channel[monitor_index]])
                m_Cmd_Status[monitor_index] = static_cast<quint16>(CMD_LOADER_REQUESTDATA::GETALL);
            ProcessCmdbuf(LOADER_CMD_SEQUENCE_CC, m_parent->m_STC_Comm_SendData + m_parent->m_parent->m_Loader_Channel[monitor_index]);
            break;
        case static_cast<quint16>(CMD_LOADER_REQUESTDATA::SEQUENCE_CV):
            if(m_parent->m_Comm_Response_Check[m_parent->m_parent->m_Loader_Channel[monitor_index]])
                m_Cmd_Status[monitor_index] = static_cast<quint16>(CMD_LOADER_REQUESTDATA::GETALL);
            ProcessCmdbuf(LOADER_CMD_SEQUENCE_CV, m_parent->m_STC_Comm_SendData + m_parent->m_parent->m_Loader_Channel[monitor_index]);
            break;
        case static_cast<quint16>(CMD_LOADER_REQUESTDATA::GETALL):
            ProcessCmdbuf(LOADER_CMD_GET_ALL, m_parent->m_STC_Comm_SendData + m_parent->m_parent->m_Loader_Channel[monitor_index]);
            if(m_parent->m_parent->m_ptrMonitor[monitor_index] == nullptr)
                m_Cmd_Status[monitor_index] = static_cast<quint16>(CMD_LOADER_REQUESTDATA::NONE);
            break;

        default:
            break;
        }

        if(++monitor_index >= LOADER_CHANNEL)
            monitor_index = 0;


        msleep(LOADER_REQUEST_TIMERCLOCK);
    }

    m_bCheck_Thread_Quit = true;
}

void CThread_Loader_ProcessCmdbuf::ProcessCmdbuf(quint16 Cmd, STC_COMM_LOADER* SendData)
{
    switch(Cmd)
    {
    case LOADER_CMD_SET_CHANNEL:
    {
        QByteArray arrBuf = QString::asprintf(":INST:NSEL %d\r\n", SendData->Channel + 1).toLocal8Bit();
        const char* CmdBuf = arrBuf.data();
        m_parent->m_Comm_Response_Check[SendData->Channel] = 0;
        emit ReadySendData(CmdBuf, arrBuf.length(), SendData->Channel);
    }
        break;

    case LOADER_CMD_GET_ALL:
    {
        QByteArray arrBuf = QString::asprintf(":INST:NSEL %d;:INST:NSEL?;:FUNC?;:MEAS:CURR?;VOLT?;POW?;ETIM?;:CURR?;:VOLT?;:SYST:ERR?\r\n", SendData->Channel + 1).toLocal8Bit();
        const char* CmdBuf = arrBuf.data();
        m_parent->m_Comm_Response_Check[SendData->Channel] = 0;
        emit ReadySendData(CmdBuf, arrBuf.length(), SendData->Channel);
    }
        break;

    case LOADER_CMD_GET_MEAS:
    {
        QByteArray arrBuf = QString::asprintf(":INST:NSEL %d;:MEAS:CURR?;VOLT?;POW?;ETIM?\r\n", SendData->Channel + 1).toLocal8Bit();
        const char* CmdBuf = arrBuf.data();
        m_parent->m_Comm_Response_Check[SendData->Channel] = 0;
        emit ReadySendData(CmdBuf, arrBuf.length(), SendData->Channel);
    }
        break;

    case LOADER_CMD_GET_MODE:
    {
        QByteArray arrBuf = QString::asprintf(":INST:NSEL %d;:FUNC?\r\n").toLocal8Bit();
        const char* CmdBuf = arrBuf.data();
        m_parent->m_Comm_Response_Check[SendData->Channel] = 0;
        emit ReadySendData(CmdBuf, arrBuf.length(), SendData->Channel);
    }
        break;

    case LOADER_CMD_SET_MODE:
    {
        QByteArray arrBuf = QString::asprintf(":INST:NSEL %d;:FUNC:MODE %s;\r\n", SendData->Channel + 1, SendData->Mode.toLocal8Bit().data()).toLocal8Bit();
        const char* CmdBuf = arrBuf.data();
        m_parent->m_Comm_Response_Check[SendData->Channel] = 0;
        emit ReadySendData(CmdBuf, arrBuf.length(), SendData->Channel);
    }
        break;

    case LOADER_CMD_SET_CURR:
    {
        QByteArray arrBuf = QString::asprintf(":INST:NSEL %d;:CURR %.3lf\r\n", SendData->Channel + 1, SendData->Meas.Curr).toLocal8Bit();
        const char* CmdBuf = arrBuf.data();
        m_parent->m_Comm_Response_Check[SendData->Channel] = 0;
        emit ReadySendData(CmdBuf, arrBuf.length(), SendData->Channel);
    }
        break;

    case LOADER_CMD_SET_VOLT:
    {
        QByteArray arrBuf = QString::asprintf(":INST:NSEL %d;:VOLT %.3lf\r\n", SendData->Channel + 1, SendData->Meas.Volt).toLocal8Bit();
        const char* CmdBuf = arrBuf.data();
        m_parent->m_Comm_Response_Check[SendData->Channel] = 0;
        emit ReadySendData(CmdBuf, arrBuf.length(), SendData->Channel);
    }
        break;

    case LOADER_CMD_SET_RANGE_CURR:
    {
        QByteArray arrBuf = QString::asprintf(":INST:NSEL %d;:CURR:RANG %s\r\n", SendData->Channel + 1, SendData->Conf.Curr_Range.toLocal8Bit().data()).toLocal8Bit();
        const char* CmdBuf = arrBuf.data();
        m_parent->m_Comm_Response_Check[SendData->Channel] = 0;
        emit ReadySendData(CmdBuf, arrBuf.length(), SendData->Channel);
    }
        break;

    case LOADER_CMD_SET_RANGE_VOLT:
    {
        QByteArray arrBuf = QString::asprintf(":INST:NSEL %d;:VOLT:RANG %s\r\n", SendData->Channel + 1, SendData->Conf.Volt_Range.toLocal8Bit().data()).toLocal8Bit();
        const char* CmdBuf = arrBuf.data();
        m_parent->m_Comm_Response_Check[SendData->Channel] = 0;
        emit ReadySendData(CmdBuf, arrBuf.length(), SendData->Channel);
    }
        break;

    case LOADER_CMD_SEQUENCE_CC:
    {
        QByteArray arrBuf = QString::asprintf(":INST:NSEL %d;:CURR %.3lf;:INP ON;\r\n", SendData->Channel + 1, SendData->Meas.Curr).toLocal8Bit();
        const char* CmdBuf = arrBuf.data();
        m_parent->m_Comm_Response_Check[SendData->Channel] = 0;
        emit ReadySendData(CmdBuf, arrBuf.length(), SendData->Channel);
    }
        break;

    case LOADER_CMD_SEQUENCE_CV:
    {
        QByteArray arrBuf = QString::asprintf(":INST:NSEL %d;:VOLT %.3lf;:INP ON;\r\n", SendData->Channel + 1, SendData->Meas.Volt).toLocal8Bit();
        const char* CmdBuf = arrBuf.data();
        m_parent->m_Comm_Response_Check[SendData->Channel] = 0;
        emit ReadySendData(CmdBuf, arrBuf.length(), SendData->Channel);
    }
        break;

    case LOADER_CMD_ON:
    {
        QByteArray arrBuf = QString::asprintf(":INST:NSEL %d;:INP ON\r\n", SendData->Channel + 1).toLocal8Bit();
        const char* CmdBuf = arrBuf.data();
        m_parent->m_Comm_Response_Check[SendData->Channel] = 0;
        emit ReadySendData(CmdBuf, arrBuf.length(), SendData->Channel);
    }
        break;

    case LOADER_CMD_OFF:
    {
        QByteArray arrBuf = QString::asprintf(":INST:NSEL %d;:INP OFF\r\n", SendData->Channel + 1).toLocal8Bit();
        const char* CmdBuf = arrBuf.data();
        m_parent->m_Comm_Response_Check[SendData->Channel] = 0;
        emit ReadySendData(CmdBuf, arrBuf.length(), SendData->Channel);
    }
        break;

    case LOADER_CMD_GET_ERR:
    {
        QByteArray arrBuf = QString::asprintf(":SYST:ERR?\r\n", SendData->Channel + 1).toLocal8Bit();
        const char* CmdBuf = arrBuf.data();
        m_parent->m_Comm_Response_Check[SendData->Channel] = 0;
        emit ReadySendData(CmdBuf, arrBuf.length(), SendData->Channel);
    }
        break;

    default:
        break;
    }
}

void CThread_Loader_ProcessCmdbuf::SetCmdStatus(quint8 index, quint16 status)
{
    m_Cmd_Status[index] = status;
}
