#ifndef CTHREAD_FLOW_CONTROLLER_PROCESSCOMMBUF_H
#define CTHREAD_FLOW_CONTROLLER_PROCESSCOMMBUF_H

#include <QThread>
#include <QString>
#include <QMutex>
#include <QDebug>

class CQueue;
class CFlow_Controller;
class CThread_Flow_Controller_ProcessCommbuf : public QThread
{
    Q_OBJECT

public:
    CThread_Flow_Controller_ProcessCommbuf(CFlow_Controller* parent, CQueue* queue);
    ~CThread_Flow_Controller_ProcessCommbuf();

public slots:


public:     // member
    CFlow_Controller* m_parent;
    CQueue* m_Queue;

    bool m_bFlag_Thread_Quit;
    bool m_bCheck_Thread_Quit;

    // Mutex
    QMutex m_Mutex;

public:     // methods
    void Init();
    void StopThread();
    void ProcessCommbuf();
    void ByteSwapping(uint8_t* buf, uint64_t len);
    uint8_t CalCheckSum(uint8_t* buf, uint8_t len, uint64_t sum);
private:
    void run();

};

#endif // CTHREAD_FLOW_CONTROLLER_PROCESSCOMMBUF_H
