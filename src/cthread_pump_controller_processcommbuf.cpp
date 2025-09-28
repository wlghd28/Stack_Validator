#include "cthread_pump_controller_processcommbuf.h"
#include "cpump_controller.h"

CThread_Pump_Controller_ProcessCommbuf::CThread_Pump_Controller_ProcessCommbuf(CPump_Controller* parent, CQueue* queue)
{
    m_parent = parent;
    m_Queue = queue;

    Init();
}

CThread_Pump_Controller_ProcessCommbuf::~CThread_Pump_Controller_ProcessCommbuf()
{
}

/*
    override methods
*/
void CThread_Pump_Controller_ProcessCommbuf::run()
{
    ProcessCommbuf();
}

/*
    methods
*/
void CThread_Pump_Controller_ProcessCommbuf::Init()
{
    m_bFlag_Thread_Quit = false;
    m_bCheck_Thread_Quit = false;
}

void CThread_Pump_Controller_ProcessCommbuf::StopThread()
{
    m_bFlag_Thread_Quit = true;
    while(!m_bCheck_Thread_Quit){};
    quit();
    wait();
}

void CThread_Pump_Controller_ProcessCommbuf::ProcessCommbuf()
{
    quint8 TempBuf[MAXBUFSIZE]; // Temp Data array

    while(!m_bFlag_Thread_Quit)
    {
        msleep(50);

        uint16_t bufsize = m_Queue->GetDataLength();

        // 버퍼에 데이터가 없는 경우
        if(!bufsize) continue;

        size_t nr = 0;
        uint16_t index = 0;
        QStringList strarr0;

        // 버퍼 메모리 초기화
        memset(TempBuf, 0, bufsize * sizeof(uint8_t));

        // 버퍼 메모리에 메모리 복사 및 Start, Stop 비트 조사
        index = m_Queue->GetTail();
        for(int i = 0; i < bufsize; i++)
        {
            TempBuf[i] = m_Queue->GetData(index++);
            nr += (TempBuf[i] == '\n');
            index &= (MAXBUFSIZE - 1);
        }

        // '\n'이 단 한개도 없으면 무시
        if(!nr) continue;

        // Command(m_TempBuf) 파싱 순서
        // 1. "\n" 분할
        QString strtemp((char*)TempBuf);

        strarr0 = strtemp.split("\n");
        //qDebug() << strarr0[0];

        switch(m_parent->m_Cmd_Status_RequestData)
        {
        case static_cast<quint16>(CMD_PUMPCONTROLLER_REQUESTDATA::SETPOINT):
        {
            m_parent->m_Comm_Response_Check = 1;
        }
            break;

        case static_cast<quint16>(CMD_PUMPCONTROLLER_REQUESTDATA::GETPOINT):
        {
            m_parent->m_Comm_Response_Check = 1;
        }
            break;

        case static_cast<quint16>(CMD_PUMPCONTROLLER_REQUESTDATA::GETMEAS):
        {
            m_parent->m_Comm_Response_Check = 1;
        }
            break;

        default:
            break;
        }

        m_Queue->Delete((uint16_t)(strlen(strarr0[0].toLocal8Bit().data())) + 1);
    }

    m_bCheck_Thread_Quit = true;
}
