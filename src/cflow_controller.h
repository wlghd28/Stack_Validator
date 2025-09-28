#ifndef CFLOW_CONTROLLER_H
#define CFLOW_CONTROLLER_H

#include <QObject>
#include <QtSerialPort/QSerialPort>
#include <QtSerialPort/QSerialPortInfo>
#include <QString>
#include <QTimer>
#include <QThread>
#include <QMutex>

#include "cthread_flow_controller_processcmdbuf.h"
#include "cthread_flow_controller_processcommbuf.h"
#include "cqueue.h"
#include "comm_protocol.h"
#include "macro_define.h"

class Stack_Validator;
class CFlow_Controller : public QObject
{
    Q_OBJECT

public:
    CFlow_Controller(Stack_Validator* parent);
    ~CFlow_Controller();

public slots:   // Serial
    void RecvDataFromSerial();
    int SendDataToSerial(const char* data, quint64 len);
    void SerialError(QSerialPort::SerialPortError error);

public slots:   // timer
    void OnTimerReconnectToSerial();

public:
    // thread
    CThread_Flow_Controller_ProcessCmdbuf* m_ptrThread_ProcessCmdbuf;
    CThread_Flow_Controller_ProcessCommbuf* m_ptrThread_ProcessCommbuf;

    // members
    Stack_Validator*  m_parent;   // UI Object
    QSerialPort*  m_SerialPort;
    quint8 m_Check_SerialPort_isOpen; // 0 : close, 1 : open
    CQueue m_Queue; // Data Queue

    // protocol data
    STC_FLOW_CONTROLLER_MISO m_STC_Comm_MISO;
    STC_FLOW_CONTROLLER_MOSI m_STC_Comm_MOSI;
    STC_COMM_FLOW_CONTROLLER_RECV m_STC_Comm_RecvData[FLOWCONTROLLER_ADDRESS];  // 0 ~ 4 : Cathode, 5 ~ 9 : N2
    STC_COMM_FLOW_CONTROLLER_SEND m_STC_Comm_SendData[FLOWCONTROLLER_ADDRESS]; // 0 ~ 4 : Cathode, 5 ~ 9 : N2

    // Flow Ctrl Address, Monitor Matching
    QHash<quint8, quint8> m_Hash_AddressToMonitor;

    // Mutex
    QMutex m_Mutex;

    quint8 m_Comm_Response_Check[FLOWCONTROLLER_ADDRESS]; // Val >> 0 : No, 1 : OK | Index >> 0 ~ 4 : Cathode, 5 ~ 9 : N2

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

    void InitFlowControllerSetting(quint8 index);
    void ResetFlowController(quint8 index, quint8 controller);
    void GetFlowControllerPoint(quint8 index);
    void SetFlowControllerPoint(quint8 index, double val_ca, double val_n2);
    void GetFlowControllerMeas(quint8 index);
    void ErrHandleFlowController(quint8 index, quint8 controller);
};

#endif // CFLOW_CONTROLLER_H
