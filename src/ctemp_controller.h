#ifndef CTEMP_CONTROLLER_H
#define CTEMP_CONTROLLER_H

#include <QObject>
#include <QtSerialPort/QSerialPort>
#include <QtSerialPort/QSerialPortInfo>
#include <QString>
#include <QTimer>
#include <QThread>
#include <QMutex>

#include "cthread_temp_controller_processcmdbuf.h"
#include "cthread_temp_controller_processcommbuf.h"
#include "cqueue.h"
#include "comm_protocol.h"
#include "macro_define.h"

class Stack_Validator;
class CTemp_Controller : public QObject
{
    Q_OBJECT
public:
    CTemp_Controller(Stack_Validator* parent);
    ~CTemp_Controller();

public slots:   // Serial
    void RecvDataFromSerial();
    int SendDataToSerial(const char* data, quint64 len);
    void SerialError(QSerialPort::SerialPortError error);

public slots:   // timer
    void OnTimerReconnectToSerial();

public:
    // thread
    CThread_Temp_Controller_ProcessCmdbuf* m_ptrThread_ProcessCmdbuf;
    CThread_Temp_Controller_ProcessCommbuf* m_ptrThread_ProcessCommbuf;

    // members
    Stack_Validator*  m_parent;   // UI Object
    QSerialPort*  m_SerialPort;
    quint8 m_Check_SerialPort_isOpen; // 0 : close, 1 : open
    CQueue m_Queue; // Data Queue

    // protocol data
    STC_TEMP_CONTROLLER_MISO m_STC_Comm_MISO;
    STC_TEMP_CONTROLLER_MOSI m_STC_Comm_MOSI;
    STC_COMM_TEMP_CONTROLLER_RECV m_STC_Comm_RecvData[TEMPCONTROLLER_ADDRESS];  //
    STC_COMM_TEMP_CONTROLLER_SEND m_STC_Comm_SendData[TEMPCONTROLLER_ADDRESS]; //

    // Temp Ctrl Address, Monitor Matching
    QHash<quint8, quint8> m_Hash_AddressToMonitor;

    // Mutex
    QMutex m_Mutex;

    quint8 m_Comm_Response_Check[TEMPCONTROLLER_ADDRESS]; // Val >> 0 : No, 1 : OK
    quint8 m_Cmd_Status_RequestData[TEMPCONTROLLER_ADDRESS];

    // Timer
    std::shared_ptr<QTimer> m_pTimer_ReconnectToSerial;

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

    void InitTempControllerSetting(quint8 index);
    void GetTempControllerPoint(quint8 index);
    void SetTempControllerPoint(quint8 index, double val);
    void GetTempControllerMeas(quint8 index);
    void ErrHandleTempController(quint8 index);
};

#endif // CTEMP_CONTROLLER_H
