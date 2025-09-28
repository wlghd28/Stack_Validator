#ifndef CTHREAD_PUMP_CONTROLLER_PROCESSCMDBUF_H
#define CTHREAD_PUMP_CONTROLLER_PROCESSCMDBUF_H

#include <QThread>
#include <QString>
#include <QMutex>
#include <QDebug>

#include "macro_define.h"
#include "comm_protocol.h"

class CPump_Controller;
class CThread_Pump_Controller_ProcessCmdbuf : public QThread
{
    Q_OBJECT
public:
    CThread_Pump_Controller_ProcessCmdbuf(CPump_Controller* parent);
    ~CThread_Pump_Controller_ProcessCmdbuf();

signals:
    int ReadySendData(const char* data, quint64 len);

public slots:

public:     // member
    CPump_Controller* m_parent;

    bool m_bFlag_Thread_Quit;
    bool m_bCheck_Thread_Quit;

    quint16 m_Cmd_Status;

    // Mutex
    QMutex m_Mutex;

public:     // methods
    void Init();
    void StopThread();
    void ProcessCmdStatus();
    void ProcessCmdbuf(quint16 Cmd, STC_COMM_PUMP_CONTROLLER* SendData);
    void SetCmdStatus(quint16 status);

private:
    void run();
};

#endif // CTHREAD_PUMP_CONTROLLER_PROCESSCMDBUF_H
