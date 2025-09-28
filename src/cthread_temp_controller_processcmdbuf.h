#ifndef CTHREAD_TEMP_CONTROLLER_PROCESSCMDBUF_H
#define CTHREAD_TEMP_CONTROLLER_PROCESSCMDBUF_H

#include <QThread>
#include <QString>
#include <QMutex>
#include <QDebug>

#include "macro_define.h"
#include "comm_protocol.h"

class CTemp_Controller;
class CThread_Temp_Controller_ProcessCmdbuf : public QThread
{
    Q_OBJECT
public:
    CThread_Temp_Controller_ProcessCmdbuf(CTemp_Controller* parent);
    ~CThread_Temp_Controller_ProcessCmdbuf();

signals:
    int ReadySendData(const char* data, quint64 len);

public slots:


public:     // member
    CTemp_Controller* m_parent;

    bool m_bFlag_Thread_Quit;
    bool m_bCheck_Thread_Quit;

    quint16 m_Cmd_Status[LOADER_CHANNEL];

    // Mutex
    QMutex m_Mutex;

public:     // methods
    void Init();
    void StopThread();
    void ProcessCmdStatus();
    void ProcessCmdbuf(quint16 Cmd, STC_COMM_TEMP_CONTROLLER_SEND* SendData);
    uint16_t CalCRC(uint8_t* cBuf, uint8_t cLen);
    void SetCmdStatus(quint8 index, quint16 status);

private:
    void run();
};

#endif // CTHREAD_TEMP_CONTROLLER_PROCESSCMDBUF_H
