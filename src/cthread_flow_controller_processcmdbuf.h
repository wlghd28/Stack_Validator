#ifndef CTHREAD_FLOW_CONTROLLER_PROCESSCMDBUF_H
#define CTHREAD_FLOW_CONTROLLER_PROCESSCMDBUF_H

#include <QThread>
#include <QString>
#include <QMutex>
#include <QDebug>

#include "macro_define.h"
#include "comm_protocol.h"

class CFlow_Controller;
class CThread_Flow_Controller_ProcessCmdbuf : public QThread
{
    Q_OBJECT
public:
    CThread_Flow_Controller_ProcessCmdbuf(CFlow_Controller* parent);
    ~CThread_Flow_Controller_ProcessCmdbuf();

signals:
    int ReadySendData(const char* data, quint64 len);
    int ReadySendData_OtherThread(const char* data, quint64 len);

public slots:

public:     // member
    CFlow_Controller* m_parent;

    bool m_bFlag_Thread_Quit;
    bool m_bCheck_Thread_Quit;

    quint16 m_Cmd_Status[LOADER_CHANNEL];

    // Mutex
    QMutex m_Mutex;

public:     // methods
    void Init();
    void StopThread();
    void ProcessCmdStatus();
    void ProcessCmdbuf(quint16 Cmd, STC_COMM_FLOW_CONTROLLER_SEND* SendData);
    void ProcessCmdbuf_OtherThread(quint16 Cmd, STC_COMM_FLOW_CONTROLLER_SEND* SendData);
    void ByteStuffing_Frame_MOSI(STC_COMM_FLOW_CONTROLLER_SEND* SendData, uint8_t* frame, uint8_t* index_frame);
    void ByteSwapping(uint8_t* buf, uint64_t len);
    uint8_t CalCheckSum(uint8_t* buf, uint8_t len, uint64_t sum);
    void SetCmdStatus(quint8 index, quint16 status);

private:
    void run();

};

#endif // CTHREAD_FLOW_CONTROLLER_PROCESSCMDBUF_H
