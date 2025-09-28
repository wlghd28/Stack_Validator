#include "cthread_loader_processcommbuf.h"
#include "cloader.h"

CThread_Loader_ProcessCommbuf::CThread_Loader_ProcessCommbuf(CLoader* parent, CQueue* queue)
{
    m_parent = parent;
    m_Queue = queue;

    Init();
}

CThread_Loader_ProcessCommbuf::~CThread_Loader_ProcessCommbuf()
{
    m_bFlag_Thread_Quit = true;
}

/*
    override methods
*/
void CThread_Loader_ProcessCommbuf::run()
{
    ProcessCommbuf();
}

/*
    methods
*/
void CThread_Loader_ProcessCommbuf::Init()
{
    m_bFlag_Thread_Quit = false;
    m_bCheck_Thread_Quit = false;
}

void CThread_Loader_ProcessCommbuf::StopThread()
{
    m_bFlag_Thread_Quit = true;
    while(!m_bCheck_Thread_Quit){};
    quit();
    wait();
}

uint16_t CThread_Loader_ProcessCommbuf::Search_StrCmd(const char** arr, char* target, uint16_t arrlen)
{
    uint16_t ret = 99;

    for(int i = 0; i < arrlen; i++)
    {
        //if(strcmp(target, arr[i]) == 0)
        if(_stricmp(target, arr[i]) == 0)
        {
            ret = i;
            break;
        }
    }

    return ret;
}

void CThread_Loader_ProcessCommbuf::ProcessCommbuf()
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

        // 버퍼 메모리에 메모리 복사 및 '\n'의 개수 조사
        index = m_Queue->GetTail();
        for(int i = 0; i < bufsize; i++)
        {
            TempBuf[i] = m_Queue->GetData(index);
            if(TempBuf[i] == '\n') nr++;
            index++;
            index &= (MAXBUFSIZE - 1);
        }

        // '\n'이 단 한개도 없으면 무시
        if(!nr) continue;

        // Command(m_TempBuf) 파싱 순서
        // 1. "\n" 분할
        QString strtemp((char*)TempBuf);

        strarr0 = strtemp.split("\n");
        //qDebug() << strarr0[0];

        const char* arrcmd0[1] =
        {
            ""
        };

        switch(Search_StrCmd(arrcmd0, strarr0[0].toLocal8Bit().data(), 1))
        {
        case 0:
        {

        }
            break;

        default: // Data
        {
            QStringList strParsingData = strarr0[0].split(";");
            if(strParsingData.length() > 8)
            {
                QStringList strParsingError = strParsingData[8].split(",");
                quint8 Ch = strParsingData[0].toUInt() - 1;
                QString Mode = strParsingData[1];
                double MeasCurr = strParsingData[2].toDouble();
                double MeasVolt = strParsingData[3].toDouble();
                double MeasPower = strParsingData[4].toDouble();
                double MeasEltm = strParsingData[5].toDouble();
                double ConfCurr = strParsingData[6].toDouble();
                double ConfVolt = strParsingData[7].toDouble();
                int Event_Num = 0;
                QString Error;

                if(strParsingError.length() > 1)
                {
                    Event_Num = strParsingError[0].toInt();
                    Error = strParsingError[1];
                }

                if(Ch >= 0 && Ch < LOADER_CHANNEL)
                {
                    m_parent->m_STC_Comm_RecvData[Ch].Channel = Ch;
                    m_parent->m_STC_Comm_RecvData[Ch].Mode = Mode;
                    m_parent->m_STC_Comm_RecvData[Ch].Meas.Curr = MeasCurr;
                    m_parent->m_STC_Comm_RecvData[Ch].Meas.Volt = MeasVolt;
                    m_parent->m_STC_Comm_RecvData[Ch].Meas.Power = MeasPower;
                    m_parent->m_STC_Comm_RecvData[Ch].Meas.ElapsedTime = MeasEltm;
                    m_parent->m_STC_Comm_RecvData[Ch].Conf.Curr = ConfCurr;
                    m_parent->m_STC_Comm_RecvData[Ch].Conf.Volt = ConfVolt;
                    m_parent->m_STC_Comm_RecvData[Ch].Err.Event_Num = Event_Num;
                    m_parent->m_STC_Comm_RecvData[Ch].Err.Error = Error;
                }
            }
        }
            break;
        }

        m_Queue->Delete((uint16_t)(strlen(strarr0[0].toLocal8Bit().data())) + 1);
    }

    m_bCheck_Thread_Quit = true;
}
