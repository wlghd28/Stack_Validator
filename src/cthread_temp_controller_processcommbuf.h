#ifndef CTHREAD_TEMP_CONTROLLER_PROCESSCOMMBUF_H
#define CTHREAD_TEMP_CONTROLLER_PROCESSCOMMBUF_H

#include <QThread>
#include <QString>
#include <QMutex>
#include <QDebug>

#include "comm_protocol.h"

class CQueue;
class CTemp_Controller;
class CThread_Temp_Controller_ProcessCommbuf : public QThread
{
    Q_OBJECT
public:
    CThread_Temp_Controller_ProcessCommbuf(CTemp_Controller* parent, CQueue* queue);
    ~CThread_Temp_Controller_ProcessCommbuf();

public slots:


public:     // member
    CTemp_Controller* m_parent;
    CQueue* m_Queue;

    bool m_bFlag_Thread_Quit;
    bool m_bCheck_Thread_Quit;

    // Mutex
    QMutex m_Mutex;

public:     // methods
    void Init();
    void StopThread();
    void ProcessCommbuf();
    uint16_t CalCRC(uint8_t* cBuf, uint8_t cLen);

private:
    void run();
};

#endif // CTHREAD_TEMP_CONTROLLER_PROCESSCOMMBUF_H
