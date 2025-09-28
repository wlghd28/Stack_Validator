#include "cpump_controller.h"
#include "Stack_Validator.h"

CPump_Controller::CPump_Controller(Stack_Validator* parent, quint8 channel)
{
    m_parent = parent;
    m_Channel = channel;
    m_Check_SerialPort_isOpen = 0;
    m_Comm_Response_Check = 0;

    // timer
    InitTimer();

    // Serial
    InitSerial();
    ConnectToSerial();

    // thread
    InitThread();

    memset(&m_STC_Comm_RecvData, 0, sizeof(m_STC_Comm_RecvData));
    memset(&m_STC_Comm_SendData, 0, sizeof(m_STC_Comm_SendData));

    m_STC_Comm_RecvData.Addr = 1;
    m_STC_Comm_SendData.Addr = 1;
}

CPump_Controller::~CPump_Controller()
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
void CPump_Controller::InitSerial()
{
    m_SerialPort = new QSerialPort(this);

    if(m_SerialPort)
    {
        connect(m_SerialPort, SIGNAL(readyRead()), this, SLOT(RecvDataFromSerial()));
        connect(m_SerialPort, SIGNAL(errorOccurred(QSerialPort::SerialPortError)), this, SLOT(SerialError(QSerialPort::SerialPortError)));
    }
}

void CPump_Controller::InitTimer()
{
    m_pTimer_ReconnectToSerial = std::make_shared<QTimer>();
    connect(m_pTimer_ReconnectToSerial.get(), SIGNAL(timeout()), this, SLOT(OnTimerReconnectToSerial()));
}

void CPump_Controller::InitThread()
{
    m_ptrThread_ProcessCmdbuf = new CThread_Pump_Controller_ProcessCmdbuf(this);

    if(m_ptrThread_ProcessCmdbuf)
    {
        connect(m_ptrThread_ProcessCmdbuf, SIGNAL(ReadySendData(const char*, quint64)), this, SLOT(SendDataToSerial(const char*, quint64)), Qt::BlockingQueuedConnection);
        connect(m_ptrThread_ProcessCmdbuf, SIGNAL(finished()), m_ptrThread_ProcessCmdbuf, SLOT(deleteLater()));
        m_ptrThread_ProcessCmdbuf->start();
    }

    m_ptrThread_ProcessCommbuf = new CThread_Pump_Controller_ProcessCommbuf(this, &m_Queue);

    if(m_ptrThread_ProcessCommbuf)
    {
        connect(m_ptrThread_ProcessCommbuf, SIGNAL(finished()), m_ptrThread_ProcessCommbuf, SLOT(deleteLater()));
        m_ptrThread_ProcessCommbuf->start();
    }
}

QList<QSerialPortInfo> CPump_Controller::GetPortsInfo()
{
    const auto infos = QSerialPortInfo::availablePorts();
    return infos;
}

void CPump_Controller::ConnectToSerial()
{
    if(m_SerialPort)
    {
        QSettings settings(m_parent->m_strName_Enterprise, m_parent->m_strName_Program);

        // Connect Serial
        int Ret = 0;
        Ret = StartSerialPort
        (
            settings.value(QString::asprintf("Config/SER/Pump_Ctrl.%d/Port", m_Channel + 1)).toString(),
            settings.value(QString::asprintf("Config/SER/Pump_Ctrl.%d/Baudrate", m_Channel + 1)).toUInt(),
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
                    m_parent->m_ptrMonitor[i]->SetPumpControllerLED();
                }
            }

            m_pTimer_ReconnectToSerial->stop();
        }
        else
        {
            m_parent->SaveLogFile(m_parent->m_strLogFileName_Communication, "Fail to connect Pump Controller comport");
            qDebug() << "Fail to connect Pump Controller comport";
        }
    }
}

int CPump_Controller::StartSerialPort
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

int CPump_Controller::StopSerialPort()
{
    if(m_Check_SerialPort_isOpen)
        m_SerialPort->close();

    m_Check_SerialPort_isOpen = 0;

    for(int i = 0; i < LOADER_CHANNEL; i++)
    {
        if(m_parent->m_ptrMonitor[i] != nullptr)
        {
            m_parent->m_ptrMonitor[i]->SetPumpControllerLED();
        }
    }

    return 0;
}

void CPump_Controller::InitPumpControllerSetting()
{
    GetPumpControllerMeas();
}

void CPump_Controller::GetPumpControllerPoint()
{
    if(!(m_parent->m_PumpController_Model[m_Channel] < PUMP_CHANNEL)) return;

    m_ptrThread_ProcessCmdbuf->SetCmdStatus(static_cast<quint16>(CMD_PUMPCONTROLLER_REQUESTDATA::GETPOINT));
    m_parent->m_ptrPump_Controller[m_parent->m_PumpController_Model[m_Channel]]->m_Comm_Response_Check = 0;
}

void CPump_Controller::SetPumpControllerAnodePump(double val)
{
    if(!(m_parent->m_PumpController_Model[m_Channel] < PUMP_CHANNEL)) return;

    m_parent->m_ptrPump_Controller[m_parent->m_PumpController_Model[m_Channel]]->m_STC_Comm_SendData.FlowRate = val;
    m_parent->m_ptrPump_Controller[m_parent->m_PumpController_Model[m_Channel]]->m_STC_Comm_SendData.Rpm = (quint16)(round(val * (double)100));
    m_ptrThread_ProcessCmdbuf->SetCmdStatus(static_cast<quint16>(CMD_PUMPCONTROLLER_REQUESTDATA::SETPOINT));
    m_parent->m_ptrPump_Controller[m_parent->m_PumpController_Model[m_Channel]]->m_Comm_Response_Check = 0;
}

void CPump_Controller::GetPumpControllerMeas()
{
    if(!(m_parent->m_PumpController_Model[m_Channel] < PUMP_CHANNEL)) return;

    m_ptrThread_ProcessCmdbuf->SetCmdStatus(static_cast<quint16>(CMD_PUMPCONTROLLER_REQUESTDATA::GETMEAS));
    m_parent->m_ptrPump_Controller[m_parent->m_PumpController_Model[m_Channel]]->m_Comm_Response_Check = 0;
}

/*
    Serial slots
*/
void CPump_Controller::RecvDataFromSerial()
{
    const QByteArray buffer = m_SerialPort->readAll();
    const char* data = buffer.data();

    for(int i = 0; i < buffer.length(); i++)
    {
        if(data[i] == '*' || data[i] == '#')
            m_Comm_Response_Check = 1;
        else
            m_Queue.Insert(data[i]);
    }

//    qDebug() << "Serial RecvTest";
//    qDebug() << "size : " << buffer.size();
//    qDebug() << "data : " << buffer.data();
}

int CPump_Controller::SendDataToSerial(const char* data, quint64 len)
{
    if(m_SerialPort == nullptr) return SOCKET_ERROR;
    if(!m_Check_SerialPort_isOpen) return SOCKET_ERROR;
    if(m_SerialPort->bytesToWrite() > 0) m_SerialPort->clear(QSerialPort::Direction::Output);

    int Ret = SOCKET_ERROR;

    Ret = m_SerialPort->write(reinterpret_cast<const char*>(data), len);
    m_SerialPort->waitForBytesWritten(WAIT_FOR_BYTE_WRITTEN);

    return Ret;
}

void CPump_Controller::SerialError(QSerialPort::SerialPortError error)
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

    //m_parent->SaveLogFile(m_parent->m_strLogFileName_Communication, "Pump Ctrl:" + m_SerialPort->errorString() + ":" + QString(error));
    qDebug() << m_SerialPort->errorString() << ":" << QString(error);
}

/*
    timer slots
*/
void CPump_Controller::OnTimerReconnectToSerial()
{
    ConnectToSerial();
}
