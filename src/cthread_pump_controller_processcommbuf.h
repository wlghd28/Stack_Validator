#ifndef CTHREAD_PUMP_CONTROLLER_PROCESSCOMMBUF_H
#define CTHREAD_PUMP_CONTROLLER_PROCESSCOMMBUF_H

#include <QThread>
#include <QString>
#include <QMutex>
#include <QDebug>

class CQueue;
class CPump_Controller;
class CThread_Pump_Controller_ProcessCommbuf : public QThread
{
    Q_OBJECT
public:
    CThread_Pump_Controller_ProcessCommbuf(CPump_Controller* parent, CQueue* queue);
    ~CThread_Pump_Controller_ProcessCommbuf();

public slots:

public:     // member
    CPump_Controller* m_parent;
    CQueue* m_Queue;

    bool m_bFlag_Thread_Quit;
    bool m_bCheck_Thread_Quit;

    // Mutex
    QMutex m_Mutex;

public:     // methods
    void Init();
    void StopThread();
    void ProcessCommbuf();

private:
    void run();
};

#endif // CTHREAD_PUMP_CONTROLLER_PROCESSCOMMBUF_H
