#include "cthread_pump_controller_processcmdbuf.h"
#include "Stack_Validator.h"

CThread_Pump_Controller_ProcessCmdbuf::CThread_Pump_Controller_ProcessCmdbuf(CPump_Controller* parent)
{
    m_parent = parent;

    Init();
}

CThread_Pump_Controller_ProcessCmdbuf::~CThread_Pump_Controller_ProcessCmdbuf()
{
}

/*
    override methods
*/
void CThread_Pump_Controller_ProcessCmdbuf::run()
{
    ProcessCmdStatus();
}

/*
    methods
*/
void CThread_Pump_Controller_ProcessCmdbuf::Init()
{
    m_bFlag_Thread_Quit = false;
    m_bCheck_Thread_Quit = false;
    m_Cmd_Status = static_cast<quint16>(CMD_FLOWCONTROLLER_REQUESTDATA::NONE);
}

void CThread_Pump_Controller_ProcessCmdbuf::StopThread()
{
    m_bFlag_Thread_Quit = true;
    while(!m_bCheck_Thread_Quit){};
    quit();
    wait();
}

void CThread_Pump_Controller_ProcessCmdbuf::ProcessCmdStatus()
{
    CPump_Controller* ptrPump_Controller = nullptr;

    while(!m_bFlag_Thread_Quit)
    {
        if(!(m_parent->m_parent->m_PumpController_Model[m_parent->m_Channel] < PUMP_CHANNEL)) continue;

        ptrPump_Controller = m_parent->m_parent->m_ptrPump_Controller[m_parent->m_parent->m_PumpController_Model[m_parent->m_Channel]];

        switch(m_Cmd_Status)
        {
        case static_cast<quint16>(CMD_PUMPCONTROLLER_REQUESTDATA::SETPOINT):
            if(ptrPump_Controller->m_Comm_Response_Check)
            {
                m_parent->GetPumpControllerMeas();
                ptrPump_Controller->m_STC_Comm_RecvData.Rpm = ptrPump_Controller->m_STC_Comm_SendData.Rpm;
                ptrPump_Controller->m_Cmd_Status_RequestData = static_cast<quint16>(CMD_PUMPCONTROLLER_REQUESTDATA::GETMEAS);
                ptrPump_Controller->m_ptrThread_ProcessCmdbuf->ProcessCmdbuf(PUMP_CONTROLLER_CMD_GET_MEAS, &(ptrPump_Controller->m_STC_Comm_SendData));
            }
            else
            {
                ptrPump_Controller->m_Cmd_Status_RequestData = static_cast<quint16>(CMD_PUMPCONTROLLER_REQUESTDATA::SETPOINT);
                if(ptrPump_Controller->m_STC_Comm_SendData.Rpm > 0)
                    ptrPump_Controller->m_ptrThread_ProcessCmdbuf->ProcessCmdbuf(PUMP_CONTROLLER_CMD_START, &(ptrPump_Controller->m_STC_Comm_SendData));
                else
                    ptrPump_Controller->m_ptrThread_ProcessCmdbuf->ProcessCmdbuf(PUMP_CONTROLLER_CMD_STOP, &(ptrPump_Controller->m_STC_Comm_SendData));
            }
            break;

        case static_cast<quint16>(CMD_PUMPCONTROLLER_REQUESTDATA::GETPOINT):
            if(ptrPump_Controller->m_Comm_Response_Check)
            {
                m_parent->GetPumpControllerMeas();
                ptrPump_Controller->m_Cmd_Status_RequestData = static_cast<quint16>(CMD_PUMPCONTROLLER_REQUESTDATA::GETMEAS);
                ptrPump_Controller->m_ptrThread_ProcessCmdbuf->ProcessCmdbuf(PUMP_CONTROLLER_CMD_GET_MEAS, &(ptrPump_Controller->m_STC_Comm_SendData));
            }
            else
            {
                ptrPump_Controller->m_Cmd_Status_RequestData = static_cast<quint16>(CMD_PUMPCONTROLLER_REQUESTDATA::GETPOINT);
                ptrPump_Controller->m_ptrThread_ProcessCmdbuf->ProcessCmdbuf(PUMP_CONTROLLER_CMD_GET_POINT, &(ptrPump_Controller->m_STC_Comm_SendData));
            }
            break;

        case static_cast<quint16>(CMD_PUMPCONTROLLER_REQUESTDATA::GETMEAS):
            ptrPump_Controller->m_Cmd_Status_RequestData = static_cast<quint16>(CMD_PUMPCONTROLLER_REQUESTDATA::GETMEAS);
            ptrPump_Controller->m_ptrThread_ProcessCmdbuf->ProcessCmdbuf(PUMP_CONTROLLER_CMD_GET_MEAS, &(ptrPump_Controller->m_STC_Comm_SendData));
            if(m_parent->m_parent->m_ptrMonitor[m_parent->m_Channel] == nullptr)
                m_Cmd_Status = static_cast<quint16>(CMD_PUMPCONTROLLER_REQUESTDATA::NONE);
            break;

        default:
            break;
        }


        msleep(PUMPCONTROLLER_REQUEST_TIMERCLOCK);
    }

    m_bCheck_Thread_Quit = true;
}

void CThread_Pump_Controller_ProcessCmdbuf::ProcessCmdbuf(quint16 Cmd, STC_COMM_PUMP_CONTROLLER* SendData)
{
    switch(Cmd)
    {
    case PUMP_CONTROLLER_CMD_START:
    {
        QByteArray arrBuf = QString::asprintf("%dS%04d\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n%dH\r\n", SendData->Addr, SendData->Rpm, SendData->Addr).toLocal8Bit();
        const char* CmdBuf = arrBuf.data();
        emit ReadySendData(reinterpret_cast<const char*>(CmdBuf), arrBuf.length());
    }
        break;

    case PUMP_CONTROLLER_CMD_STOP:
    {
        QByteArray arrBuf = QString::asprintf("%dI\r\n", SendData->Addr).toLocal8Bit();
        const char* CmdBuf = arrBuf.data();
        emit ReadySendData(reinterpret_cast<const char*>(CmdBuf), arrBuf.length());
    }
        break;

    case PUMP_CONTROLLER_CMD_SET_MODE:
    {
        QString arrMode[9] = { "L", "M", "N", "O", "]", "P", "Q", "G", "R" };
        QByteArray arrBuf = QString::asprintf("%d%s\r\n", SendData->Addr, arrMode[SendData->Mode].toLocal8Bit().data()).toLocal8Bit();
        const char* CmdBuf = arrBuf.data();
        emit ReadySendData(reinterpret_cast<const char*>(CmdBuf), arrBuf.length());
    }
        break;

    case PUMP_CONTROLLER_CMD_SET_POINT:
    {
        QByteArray arrBuf = QString::asprintf("%dS%04d\r\n", SendData->Addr, SendData->Rpm).toLocal8Bit();
        const char* CmdBuf = arrBuf.data();
        emit ReadySendData(reinterpret_cast<const char*>(CmdBuf), arrBuf.length());
    }
        break;

    case PUMP_CONTROLLER_CMD_GET_POINT:
    {

    }
        break;

    case PUMP_CONTROLLER_CMD_GET_MEAS:
    {

    }
        break;

    default:
        break;
    }
}

void CThread_Pump_Controller_ProcessCmdbuf::SetCmdStatus(quint16 status)
{
    m_Cmd_Status = status;
}

