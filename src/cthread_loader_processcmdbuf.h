#ifndef CTHREAD_LOADER_PROCESSCMDBUF_H
#define CTHREAD_LOADER_PROCESSCMDBUF_H

#include <QThread>
#include <QString>
#include <QMutex>
#include <QDebug>

#include "macro_define.h"
#include "comm_protocol.h"

class CLoader;
class CThread_Loader_ProcessCmdbuf : public QThread
{
    Q_OBJECT
public:
    CThread_Loader_ProcessCmdbuf(CLoader* parent);
    ~CThread_Loader_ProcessCmdbuf();

signals:
    int ReadySendData(const char* data, quint64 len, quint8 channel);

public slots:

public:     // member
    CLoader* m_parent;

    bool m_bFlag_Thread_Quit;
    bool m_bCheck_Thread_Quit;

    quint16 m_Cmd_Status[LOADER_CHANNEL];

    // Mutex
    QMutex m_Mutex;

public:     // methods
    void Init();
    void StopThread();
    void ProcessCmdStatus();
    void ProcessCmdbuf(quint16 Cmd, STC_COMM_LOADER* SendData);
    void SetCmdStatus(quint8 index, quint16 status);

private:
    void run();
};

#endif // CTHREAD_LOADER_PROCESSCMDBUF_H
