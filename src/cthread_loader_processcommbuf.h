#ifndef CTHREAD_LOADER_PROCESSCOMMBUF_H
#define CTHREAD_LOADER_PROCESSCOMMBUF_H

#include <QThread>
#include <QString>
#include <QMutex>
#include <QDebug>

class CQueue;
class CLoader;
class CThread_Loader_ProcessCommbuf : public QThread
{
    Q_OBJECT

public:
    CThread_Loader_ProcessCommbuf(CLoader* parent, CQueue* queue);
    ~CThread_Loader_ProcessCommbuf();


public slots:


public:     // member
    CLoader* m_parent;
    CQueue* m_Queue;

    bool m_bFlag_Thread_Quit;
    bool m_bCheck_Thread_Quit;

    // Mutex
    QMutex m_Mutex;

public:     // methods
    void Init();
    void StopThread();
    uint16_t Search_StrCmd(const char** arr, char* target, uint16_t arrlen);
    void ProcessCommbuf();

private:
    void run();
};

#endif // CTHREAD_LOADER_PROCESSCOMMBUF_H
