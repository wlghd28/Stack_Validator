#include "cloader.h"
#include "Stack_Validator.h"

CLoader::CLoader(Stack_Validator* parent)
{
    m_parent = parent;
    m_FlowControl_Pause = 0;
    //m_Active_Monitor_Index = 0;
    m_Check_SerialPort_isOpen = 0;
    for(int i = 0; i < LOADER_CHANNEL; i++){ m_Comm_Response_Check[i] = 1; }

    // timer
    InitTimer();

    // Serial
    InitSerial();
    ConnectToSerial();

    // thread
    InitThread();
}

CLoader::~CLoader()
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
void CLoader::InitSerial()
{
    m_SerialPort = new QSerialPort(this);

    if(m_SerialPort)
    {
        connect(m_SerialPort, SIGNAL(readyRead()), this, SLOT(RecvDataFromSerial()));
        connect(m_SerialPort, SIGNAL(errorOccurred(QSerialPort::SerialPortError)), this, SLOT(SerialError(QSerialPort::SerialPortError)));
    }
}

void CLoader::InitTimer()
{
    m_pTimer_ReconnectToSerial = std::make_shared<QTimer>();
    connect(m_pTimer_ReconnectToSerial.get(), SIGNAL(timeout()), this, SLOT(OnTimerReconnectToSerial()));
}

void CLoader::InitThread()
{
    m_ptrThread_ProcessCmdbuf = new CThread_Loader_ProcessCmdbuf(this);

    if(m_ptrThread_ProcessCmdbuf)
    {
        connect(m_ptrThread_ProcessCmdbuf, SIGNAL(ReadySendData(const char*, quint64, quint8)), this, SLOT(SendDataToSerial(const char*, quint64, quint8)), Qt::BlockingQueuedConnection);
        connect(m_ptrThread_ProcessCmdbuf, SIGNAL(finished()), m_ptrThread_ProcessCmdbuf, SLOT(deleteLater()));
        m_ptrThread_ProcessCmdbuf->start();
    }

    m_ptrThread_ProcessCommbuf = new CThread_Loader_ProcessCommbuf(this, &m_Queue);

    if(m_ptrThread_ProcessCommbuf)
    {
        connect(m_ptrThread_ProcessCommbuf, SIGNAL(finished()), m_ptrThread_ProcessCommbuf, SLOT(deleteLater()));
        m_ptrThread_ProcessCommbuf->start();
    }
}

QList<QSerialPortInfo> CLoader::GetPortsInfo()
{
    const auto infos = QSerialPortInfo::availablePorts();
    return infos;
}

void CLoader::ConnectToSerial()
{
    if(m_SerialPort)
    {
        QSettings settings(m_parent->m_strName_Enterprise, m_parent->m_strName_Program);

        // Connect Serial
        int Ret = 0;
        Ret = StartSerialPort
        (
            settings.value(QString::asprintf("Config/SER/E-Load/Port")).toString(),
            settings.value(QString::asprintf("Config/SER/E-Load/Baudrate")).toUInt(),
            QSerialPort::Data8,
            QSerialPort::NoParity,
            QSerialPort::TwoStop,
            QSerialPort::FlowControl::SoftwareControl
        );

        if (Ret >= 0)
        {
            m_Check_SerialPort_isOpen = 1;

            for(int i = 0; i < LOADER_CHANNEL; i++)
            {
                if(m_parent->m_ptrMonitor[i] != nullptr)
                {
                    m_parent->m_ptrMonitor[i]->SetLoaderLED();
                }
            }

            m_pTimer_ReconnectToSerial->stop();
        }
        else
        {
            m_parent->SaveLogFile(m_parent->m_strLogFileName_Communication, "Fail to connect Loader comport");
            qDebug() << "Fail to connect Loader comport";
        }
    }
}

int CLoader::StartSerialPort
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

int CLoader::StopSerialPort()
{
    if(m_Check_SerialPort_isOpen)
        m_SerialPort->close();

    m_Check_SerialPort_isOpen = 0;

    for(int i = 0; i < LOADER_CHANNEL; i++)
    {
        if(m_parent->m_ptrMonitor[i] != nullptr)
        {
            m_parent->m_ptrMonitor[i]->SetLoaderLED();
        }
    }

    return 0;
}

void CLoader::InitLoaderSetting(quint8 index)
{
    // Channel
    m_STC_Comm_SendData[m_parent->m_Loader_Channel[index]].Channel = m_parent->m_Loader_Channel[index];

    // Loader Init Sequence
    m_ptrThread_ProcessCmdbuf->SetCmdStatus(index, static_cast<quint16>(CMD_LOADER_REQUESTDATA::SEQUENCE_INIT1));
}

void CLoader::SetLoaderSwitch(quint8 index, quint8 val)
{
    if(val)
    {
        m_ptrThread_ProcessCmdbuf->SetCmdStatus(index, static_cast<quint16>(CMD_LOADER_REQUESTDATA::ON));
    }
    else
    {
        m_ptrThread_ProcessCmdbuf->SetCmdStatus(index, static_cast<quint16>(CMD_LOADER_REQUESTDATA::OFF));
    }
}

void CLoader::SetLoaderMode(quint8 index, QString strmode, double val)
{
    if(strmode == "CC")
    {
        m_STC_Comm_SendData[m_parent->m_Loader_Channel[index]].Meas.Curr = val;
        if(strmode == m_STC_Comm_RecvData[m_parent->m_Loader_Channel[index]].Mode)
            m_ptrThread_ProcessCmdbuf->SetCmdStatus(index, static_cast<quint16>(CMD_LOADER_REQUESTDATA::SEQUENCE_CC));
        else
            m_ptrThread_ProcessCmdbuf->SetCmdStatus(index, static_cast<quint16>(CMD_LOADER_REQUESTDATA::SETMODE_CC));

    }
    else if (strmode == "CV")
    {
        m_STC_Comm_SendData[m_parent->m_Loader_Channel[index]].Meas.Volt = val;
        if(strmode == m_STC_Comm_RecvData[m_parent->m_Loader_Channel[index]].Mode)
            m_ptrThread_ProcessCmdbuf->SetCmdStatus(index, static_cast<quint16>(CMD_LOADER_REQUESTDATA::SEQUENCE_CV));
        else
            m_ptrThread_ProcessCmdbuf->SetCmdStatus(index, static_cast<quint16>(CMD_LOADER_REQUESTDATA::SETMODE_CV));
    }
}

/*
    Serial slots
*/
void CLoader::RecvDataFromSerial()
{
    const QByteArray buffer = m_SerialPort->readAll();
    const char* data = buffer.data();

    for(int i = 0; i < buffer.length(); i++)
    {
        switch(data[i])
        {
        case 0x0011:
            m_FlowControl_Pause = 0;
            break;
        case 0x0013:
            m_FlowControl_Pause = 1;
            break;
        default:
            m_Queue.Insert(data[i]);
            break;
        }
    }

//    qDebug() << "Serial RecvTest";
//    qDebug() << "size : " << buffer.size();
//    qDebug() << "data : " << buffer.data();
}

int CLoader::SendDataToSerial(const char* data, quint64 len, quint8 channel)
{
    if(m_SerialPort == nullptr) return SOCKET_ERROR;
    if(!m_Check_SerialPort_isOpen) return SOCKET_ERROR;
    if(m_FlowControl_Pause) return SOCKET_ERROR;
    if(m_SerialPort->bytesToWrite() > 0) m_SerialPort->clear(QSerialPort::Direction::Output);

    int Ret = SOCKET_ERROR;

    Ret = m_SerialPort->write(reinterpret_cast<const char*>(data), len);
    m_SerialPort->waitForBytesWritten(WAIT_FOR_BYTE_WRITTEN);
    m_Comm_Response_Check[channel] = Ret;

    return Ret;
}

void CLoader::SerialError(QSerialPort::SerialPortError error)
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

    //m_parent->SaveLogFile(m_parent->m_strLogFileName_Communication, "E-Loader:" + m_SerialPort->errorString() + ":" + QString(error));
    qDebug() << m_SerialPort->errorString() << ":" << QString(error);
}

/*
    timer slots
*/
void CLoader::OnTimerReconnectToSerial()
{
    ConnectToSerial();
}
