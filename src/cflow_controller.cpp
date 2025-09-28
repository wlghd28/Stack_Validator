#include "cflow_controller.h"
#include "Stack_Validator.h"

CFlow_Controller::CFlow_Controller(Stack_Validator* parent)
{
    m_parent = parent;
    m_Check_SerialPort_isOpen = 0;
    for(int i = 0; i < LOADER_CHANNEL; i++)
    {
        m_Comm_Response_Check[i] = 0;
        //m_Comm_Rsponse_TimeOut[i] = 0;
    }

    // timer
    InitTimer();

    // Serial
    InitSerial();
    ConnectToSerial();

    // thread
    InitThread();

    memset(&m_STC_Comm_MISO, 0, sizeof(m_STC_Comm_MISO));
    memset(&m_STC_Comm_MOSI, 0, sizeof(m_STC_Comm_MOSI));
    memset(&m_STC_Comm_RecvData, 0, sizeof(m_STC_Comm_RecvData));
    memset(&m_STC_Comm_SendData, 0, sizeof(m_STC_Comm_SendData));
}

CFlow_Controller::~CFlow_Controller()
{
    if(m_ptrThread_ProcessCmdbuf != nullptr)
    {
        m_ptrThread_ProcessCmdbuf->StopThread();
    }

    if(m_ptrThread_ProcessCommbuf != nullptr)
    {
        m_ptrThread_ProcessCommbuf->StopThread();
    }

    if(m_SerialPort != nullptr)
    {
        m_SerialPort->close();
    }
}

/*
    Methods
*/
void CFlow_Controller::InitSerial()
{
    m_SerialPort = new QSerialPort(this);

    if(m_SerialPort)
    {
        connect(m_SerialPort, SIGNAL(readyRead()), this, SLOT(RecvDataFromSerial()));
        connect(m_SerialPort, SIGNAL(errorOccurred(QSerialPort::SerialPortError)), this, SLOT(SerialError(QSerialPort::SerialPortError)));
    }
}

void CFlow_Controller::InitTimer()
{
    m_pTimer_ReconnectToSerial = std::make_shared<QTimer>();
    connect(m_pTimer_ReconnectToSerial.get(), SIGNAL(timeout()), this, SLOT(OnTimerReconnectToSerial()));
}

void CFlow_Controller::InitThread()
{
    m_ptrThread_ProcessCmdbuf = new CThread_Flow_Controller_ProcessCmdbuf(this);

    if(m_ptrThread_ProcessCmdbuf)
    {
        connect(m_ptrThread_ProcessCmdbuf, SIGNAL(ReadySendData(const char*, quint64)), this, SLOT(SendDataToSerial(const char*, quint64)), Qt::BlockingQueuedConnection);
        connect(m_ptrThread_ProcessCmdbuf, SIGNAL(ReadySendData_OtherThread(const char*, quint64)), this, SLOT(SendDataToSerial(const char*, quint64)));
        connect(m_ptrThread_ProcessCmdbuf, SIGNAL(finished()), m_ptrThread_ProcessCmdbuf, SLOT(deleteLater()));
        m_ptrThread_ProcessCmdbuf->start();
    }

    m_ptrThread_ProcessCommbuf = new CThread_Flow_Controller_ProcessCommbuf(this, &m_Queue);

    if(m_ptrThread_ProcessCommbuf)
    {
        connect(m_ptrThread_ProcessCommbuf, SIGNAL(finished()), m_ptrThread_ProcessCommbuf, SLOT(deleteLater()));
        m_ptrThread_ProcessCommbuf->start();
    }
}

QList<QSerialPortInfo> CFlow_Controller::GetPortsInfo()
{
    const auto infos = QSerialPortInfo::availablePorts();
    return infos;
}

void CFlow_Controller::ConnectToSerial()
{
    if(m_SerialPort)
    {
        QSettings settings(m_parent->m_strName_Enterprise, m_parent->m_strName_Program);

        // Connect Serial
        int Ret = 0;
        Ret = StartSerialPort
        (
            settings.value(QString::asprintf("Config/SER/Flow_Ctrl/Port")).toString(),
            settings.value(QString::asprintf("Config/SER/Flow_Ctrl/Baudrate")).toUInt(),
            QSerialPort::Data8,
            QSerialPort::NoParity,
            QSerialPort::OneStop,
            QSerialPort::FlowControl::NoFlowControl
        );

        if (Ret >= 0)
        {
            m_Check_SerialPort_isOpen = 1;

            for(int i = 0; i < LOADER_CHANNEL; i++)
            {
                if(m_parent->m_ptrMonitor[i] != nullptr)
                {
                    m_parent->m_ptrMonitor[i]->SetFlowControllerLED();
                }
            }

            m_pTimer_ReconnectToSerial->stop();
        }
        else
        {
            m_parent->SaveLogFile(m_parent->m_strLogFileName_Communication, "Fail to connect Flow Controller comport");
            qDebug() << "Fail to connect Flow Controller comport";
        }
    }
}

int CFlow_Controller::StartSerialPort
(
        QString name,
        quint32 baudrate,
        QSerialPort::DataBits databits,
        QSerialPort::Parity parity,
        QSerialPort::StopBits stopbits,
        QSerialPort::FlowControl flowcontrol
)
{
    m_SerialPort->setPortName(name);
    m_SerialPort->setBaudRate(baudrate);
    m_SerialPort->setDataBits(databits);
    m_SerialPort->setParity(parity);
    m_SerialPort->setStopBits(stopbits);
    m_SerialPort->setFlowControl(flowcontrol);

    if(m_SerialPort->open(QIODevice::ReadWrite))
    {
        return 0;
    }
    else
    {
        return -1;
    }

    return -1;
}

int CFlow_Controller::StopSerialPort()
{
    if(m_Check_SerialPort_isOpen)
        m_SerialPort->close();

    m_Check_SerialPort_isOpen = 0;

    for(int i = 0; i < LOADER_CHANNEL; i++)
    {
        if(m_parent->m_ptrMonitor[i] != nullptr)
        {
            m_parent->m_ptrMonitor[i]->SetFlowControllerLED();
        }
    }

    return 0;
}

void CFlow_Controller::InitFlowControllerSetting(quint8 index)
{
    m_STC_Comm_SendData[index].Data.Address = m_parent->m_FlowController_CathodeAddr[index];
    m_STC_Comm_SendData[index + LOADER_CHANNEL].Data.Address = m_parent->m_FlowController_AnodeAddr[index];
    m_Hash_AddressToMonitor[m_parent->m_FlowController_CathodeAddr[index]] = index;
    m_Hash_AddressToMonitor[m_parent->m_FlowController_AnodeAddr[index]] = index + LOADER_CHANNEL;

    ResetFlowController(index, static_cast<quint16>(FLOW_CONTROLLER::CATHODE));
}

void CFlow_Controller::ResetFlowController(quint8 index, quint8 controller)
{
    switch(controller)
    {
    case static_cast<quint16>(FLOW_CONTROLLER::CATHODE):
        m_ptrThread_ProcessCmdbuf->SetCmdStatus(index, static_cast<quint16>(CMD_FLOWCONTROLLER_REQUESTDATA::RESET_CATHODE));
        break;

    case static_cast<quint16>(FLOW_CONTROLLER::ANODE):
        m_ptrThread_ProcessCmdbuf->SetCmdStatus(index, static_cast<quint16>(CMD_FLOWCONTROLLER_REQUESTDATA::RESET_ANODE));
        break;

    case static_cast<quint16>(FLOW_CONTROLLER::BROADCAST):
        m_ptrThread_ProcessCmdbuf->SetCmdStatus(index, static_cast<quint16>(CMD_FLOWCONTROLLER_REQUESTDATA::RESET_BROADCAST));
        break;

    default:
        break;
    }
}

void CFlow_Controller::GetFlowControllerPoint(quint8 index)
{
    m_ptrThread_ProcessCmdbuf->SetCmdStatus(index, static_cast<quint16>(CMD_FLOWCONTROLLER_REQUESTDATA::GETPOINT_CATHODE));
    m_Comm_Response_Check[index] = 0;
}

void CFlow_Controller::SetFlowControllerPoint(quint8 index, double val_ca, double val_n2)
{
    m_STC_Comm_SendData[index].Data.Point = val_ca;
    m_STC_Comm_SendData[index + LOADER_CHANNEL].Data.Point = val_n2;
    m_ptrThread_ProcessCmdbuf->SetCmdStatus(index, static_cast<quint16>(CMD_FLOWCONTROLLER_REQUESTDATA::SETPOINT_CATHODE));
    m_Comm_Response_Check[index] = 0;
    m_Comm_Response_Check[index + LOADER_CHANNEL] = 0;
}

void CFlow_Controller::GetFlowControllerMeas(quint8 index)
{
    m_ptrThread_ProcessCmdbuf->SetCmdStatus(index, static_cast<quint16>(CMD_FLOWCONTROLLER_REQUESTDATA::GETMEAS_CATHODE));
}

void CFlow_Controller::ErrHandleFlowController(quint8 index, quint8 controller)
{
    switch(controller)
    {
    case static_cast<quint16>(FLOW_CONTROLLER::CATHODE):
        switch(m_STC_Comm_RecvData[index].Frame.State.Val)
        {
        case 0:     // Success
            break;
        case 0x01:  // Data Size Error
            qDebug() << "Data Size Error";
            m_parent->SaveLogFile(m_parent->m_strLogFileName_Monitor + QString::asprintf("%d", index + 1), "Flow Cathode : Data Size Error");
            break;
        case 0x02:  // Unknown Command Error
            qDebug() << "Unknown Command Error";
            m_parent->SaveLogFile(m_parent->m_strLogFileName_Monitor + QString::asprintf("%d", index + 1), "Flow Cathode : Unknown Command Error");
            break;
        case 0x04:  // Parameter Error
            qDebug() << "Parameter Error";
            m_parent->SaveLogFile(m_parent->m_strLogFileName_Monitor + QString::asprintf("%d", index + 1), "Flow Cathode : Parameter Error");
            break;
        case 0x29:  // I2C Nack Error
            qDebug() << "I2C Nack Error";
            m_parent->SaveLogFile(m_parent->m_strLogFileName_Monitor + QString::asprintf("%d", index + 1), "Flow Cathode : I2C Nack Error");
            break;
        case 0x2A:  // I2C Master Hold Error
            qDebug() << "I2C Master Hold Error";
            m_parent->SaveLogFile(m_parent->m_strLogFileName_Monitor + QString::asprintf("%d", index + 1), "Flow Cathode : I2C Master Hold Error");
            break;
        case 0x2B:  // I2C Crc Error
            qDebug() << "I2C Crc Error";
            m_parent->SaveLogFile(m_parent->m_strLogFileName_Monitor + QString::asprintf("%d", index + 1), "Flow Cathode : I2C Crc Error");
            break;
        case 0x2C:  // Sensor Data Write Error
            qDebug() << "Sensor Data Write Error";
            m_parent->SaveLogFile(m_parent->m_strLogFileName_Monitor + QString::asprintf("%d", index + 1), "Flow Cathode : Sensor Data Write Error");
            ResetFlowController(index, controller);
            break;
        case 0x2D:  // Sensor Measure Loop Not Running Error
            qDebug() << "Sensor Measure Loop Not Running Error";
            m_parent->SaveLogFile(m_parent->m_strLogFileName_Monitor + QString::asprintf("%d", index + 1), "Flow Cathode : Sensor Measure Loop Not Running Error");
            ResetFlowController(index, controller);
            break;
        case 0x33:  // Invalid Calibration Index Error
            qDebug() << "Invalid Calibration Index Error";
            m_parent->SaveLogFile(m_parent->m_strLogFileName_Monitor + QString::asprintf("%d", index + 1), "Flow Cathode : Invalid Calibration Index Error");
            break;
        case 0x42:  // Sensor Busy Error
            qDebug() << "Sensor Busy Error";
            m_parent->SaveLogFile(m_parent->m_strLogFileName_Monitor + QString::asprintf("%d", index + 1), "Flow Cathode : Sensor Busy Error");
            ResetFlowController(index, controller);
            break;
        case 0x43:  // Command Not Allowed In Current State
            qDebug() << "Command Not Allowed In Current State";
            m_parent->SaveLogFile(m_parent->m_strLogFileName_Monitor + QString::asprintf("%d", index + 1), "Flow Cathode : Command Not Allowed In Current State");
            break;
        case 0x7F:  // Fatal Error
            qDebug() << "Fatal Error";
            m_parent->SaveLogFile(m_parent->m_strLogFileName_Monitor + QString::asprintf("%d", index + 1), "Flow Cathode : Fatal Error");
            ResetFlowController(index, controller);
            break;
        default:
            break;
        }
        break;

    case static_cast<quint16>(FLOW_CONTROLLER::ANODE):
        break;

    default:
        break;

    }
}


/*
    Serial slots
*/
void CFlow_Controller::RecvDataFromSerial()
{
    const QByteArray buffer = m_SerialPort->readAll();
    const char* data = buffer.data();

    for(int i = 0; i < buffer.length(); i++)
    {
        m_Queue.Insert(data[i]);
    }

//    qDebug() << "Serial RecvTest";
//    qDebug() << "size : " << buffer.size();
//    qDebug() << "data : " << buffer.data();
}

int CFlow_Controller::SendDataToSerial(const char* data, quint64 len)
{
    if(m_SerialPort == nullptr) return SOCKET_ERROR;
    if(!m_Check_SerialPort_isOpen) return SOCKET_ERROR;
    if(m_SerialPort->bytesToWrite() > 0) m_SerialPort->clear(QSerialPort::Direction::Output);

    int Ret = SOCKET_ERROR;

    Ret = m_SerialPort->write(reinterpret_cast<const char*>(data), len);
    m_SerialPort->waitForBytesWritten(WAIT_FOR_BYTE_WRITTEN);

    return Ret;
}

void CFlow_Controller::SerialError(QSerialPort::SerialPortError error)
{
    switch(error)
    {
    case QSerialPort::SerialPortError::NoError:
        break;
    case QSerialPort::SerialPortError::DeviceNotFoundError:
        StopSerialPort();
        m_pTimer_ReconnectToSerial->start(5000);
        break;
    case QSerialPort::SerialPortError::PermissionError:
        StopSerialPort();
        m_pTimer_ReconnectToSerial->start(5000);
        break;
    case QSerialPort::SerialPortError::OpenError:
        StopSerialPort();
        m_pTimer_ReconnectToSerial->start(5000);
        break;
    case QSerialPort::SerialPortError::ParityError:
        StopSerialPort();
        m_pTimer_ReconnectToSerial->start(5000);
        break;
    case QSerialPort::SerialPortError::FramingError:
        StopSerialPort();
        m_pTimer_ReconnectToSerial->start(5000);
        break;
    case QSerialPort::SerialPortError::BreakConditionError:
        StopSerialPort();
        m_pTimer_ReconnectToSerial->start(5000);
        break;
    case QSerialPort::SerialPortError::WriteError:
        StopSerialPort();
        m_pTimer_ReconnectToSerial->start(5000);
        break;
    case QSerialPort::SerialPortError::ReadError:
        StopSerialPort();
        m_pTimer_ReconnectToSerial->start(5000);
        break;
    case QSerialPort::SerialPortError::ResourceError:
        StopSerialPort();
        m_pTimer_ReconnectToSerial->start(5000);
        break;
    case QSerialPort::SerialPortError::UnsupportedOperationError:
        StopSerialPort();
        m_pTimer_ReconnectToSerial->start(5000);
        break;
    case QSerialPort::SerialPortError::UnknownError:
        StopSerialPort();
        m_pTimer_ReconnectToSerial->start(5000);
        break;
    case QSerialPort::SerialPortError::TimeoutError:
        StopSerialPort();
        m_pTimer_ReconnectToSerial->start(5000);
        break;
    case QSerialPort::SerialPortError::NotOpenError:
        StopSerialPort();
        m_pTimer_ReconnectToSerial->start(5000);
        break;
    default:
        break;
    }

    //m_parent->SaveLogFile(m_parent->m_strLogFileName_Communication, "Flow Ctrl:" + m_SerialPort->errorString() + ":" + QString(error));
    qDebug() << m_SerialPort->errorString() << ":" << QString(error);
}

/*
    timer slots
*/
void CFlow_Controller::OnTimerReconnectToSerial()
{
    ConnectToSerial();
}
