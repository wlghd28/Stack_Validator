#ifndef CLOADER_H
#define CLOADER_H

#include <QObject>
#include <QtSerialPort/QSerialPort>
#include <QtSerialPort/QSerialPortInfo>
#include <QString>
#include <QTimer>
#include <QThread>

#include "cthread_loader_processcmdbuf.h"
#include "cthread_loader_processcommbuf.h"
#include "cqueue.h"
#include "comm_protocol.h"
#include "macro_define.h"

class Stack_Validator;
class CLoader : public QObject
{
    Q_OBJECT

public:
    CLoader(Stack_Validator* parent);
    ~CLoader();

public slots:   // Serial
    void RecvDataFromSerial();
    int SendDataToSerial(const char* data, quint64 len, quint8 channel);
    void SerialError(QSerialPort::SerialPortError error);

public slots:   // timer
    void OnTimerReconnectToSerial();

public:
    // thread
    CThread_Loader_ProcessCmdbuf* m_ptrThread_ProcessCmdbuf;
    CThread_Loader_ProcessCommbuf* m_ptrThread_ProcessCommbuf;

    // members
    Stack_Validator*  m_parent;   // UI Object
    QSerialPort*  m_SerialPort;
    quint8 m_Check_SerialPort_isOpen; // 0 : close, 1 : open
    CQueue m_Queue; // Data Queue
    quint8 m_FlowControl_Pause; 

    // timer
    std::shared_ptr<QTimer> m_pTimer_ReconnectToSerial;

    quint8 m_Comm_Response_Check[LOADER_CHANNEL]; // Val >> 0 : No, 1 : OK

    // protocol data
    STC_COMM_LOADER m_STC_Comm_RecvData[LOADER_CHANNEL];
    STC_COMM_LOADER m_STC_Comm_SendData[LOADER_CHANNEL];

public:
    // methods
    void InitSerial();
    void InitTimer();
    void InitThread();
    QList<QSerialPortInfo> GetPortsInfo();
    void ConnectToSerial();
    int StartSerialPort
    (
            QString name,
            quint32 baudrate,
            QSerialPort::DataBits databits,
            QSerialPort::Parity parity,
            QSerialPort::StopBits stopbits,
            QSerialPort::FlowControl flowcontrol
    );
    int StopSerialPort();

    void InitLoaderSetting(quint8 index);
    void SetLoaderSwitch(quint8 index, quint8 val);
    void SetLoaderMode(quint8 index, QString strmode, double val);
};

#endif // CLOADER_H
