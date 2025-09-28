#ifndef CPUMP_CONTROLLER_H
#define CPUMP_CONTROLLER_H

#include <QObject>
#include <QtSerialPort/QSerialPort>
#include <QtSerialPort/QSerialPortInfo>
#include <QString>
#include <QTimer>
#include <QThread>
#include <QMutex>

#include "cthread_pump_controller_processcmdbuf.h"
#include "cthread_pump_controller_processcommbuf.h"
#include "cqueue.h"
#include "comm_protocol.h"
#include "macro_define.h"

class Stack_Validator;
class CPump_Controller : public QObject
{
    Q_OBJECT
public:
    CPump_Controller(Stack_Validator* parent, quint8 channel);
    ~CPump_Controller();

public slots:   // Serial
    void RecvDataFromSerial();
    int SendDataToSerial(const char* data, quint64 len);
    void SerialError(QSerialPort::SerialPortError error);

public slots:   // timer
    void OnTimerReconnectToSerial();

public:
    // thread
    CThread_Pump_Controller_ProcessCmdbuf* m_ptrThread_ProcessCmdbuf;
    CThread_Pump_Controller_ProcessCommbuf* m_ptrThread_ProcessCommbuf;

    // members
    Stack_Validator*  m_parent;   // UI Object
    quint8 m_Channel;
    QSerialPort*  m_SerialPort;
    quint8 m_Check_SerialPort_isOpen; // 0 : close, 1 : open
    CQueue m_Queue; // Data Queue

    // protocol data
    STC_COMM_PUMP_CONTROLLER m_STC_Comm_RecvData;
    STC_COMM_PUMP_CONTROLLER m_STC_Comm_SendData;

    // Flow Ctrl Address, Monitor Matching
    QHash<quint8, quint8> m_Hash_AddressToMonitor;

    // Mutex
    QMutex m_Mutex;

    quint8 m_Comm_Response_Check; // Val >> 0 : No, 1 : OK
    quint8 m_Cmd_Status_RequestData;

    // timer
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

    void InitPumpControllerSetting();
    void GetPumpControllerPoint();
    void SetPumpControllerAnodePump(double val);
    void GetPumpControllerMeas();
};

#endif // CPUMP_CONTROLLER_H
