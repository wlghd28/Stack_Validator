#include "ctemp_controller.h"
#include "Stack_Validator.h"

CTemp_Controller::CTemp_Controller(Stack_Validator* parent)
{
    m_parent = parent;
    m_Check_SerialPort_isOpen = 0;
    for(int i = 0; i < LOADER_CHANNEL; i++)
    {
        m_Comm_Response_Check[i] = 0;
        //m_Comm_Rsponse_TimeOut[i] = 0;
        m_Cmd_Status_RequestData[i] = 0;
    }

    // timer
    InitTimer();

    // Serial
    InitSerial();
    ConnectToSerial();

    // thread
    InitThread();

    // memset
    memset(&m_STC_Comm_MISO, 0, sizeof(m_STC_Comm_MISO));
    memset(&m_STC_Comm_MOSI, 0, sizeof(m_STC_Comm_MOSI));
    memset(&m_STC_Comm_RecvData, 0, sizeof(m_STC_Comm_RecvData));
    memset(&m_STC_Comm_SendData, 0, sizeof(m_STC_Comm_SendData));
}

CTemp_Controller::~CTemp_Controller()
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
void CTemp_Controller::InitSerial()
{
    m_SerialPort = new QSerialPort(this);

    if(m_SerialPort)
    {
        connect(m_SerialPort, SIGNAL(readyRead()), this, SLOT(RecvDataFromSerial()));
        connect(m_SerialPort, SIGNAL(errorOccurred(QSerialPort::SerialPortError)), this, SLOT(SerialError(QSerialPort::SerialPortError)));
    }
}

void CTemp_Controller::InitTimer()
{
    m_pTimer_ReconnectToSerial = std::make_shared<QTimer>();
    connect(m_pTimer_ReconnectToSerial.get(), SIGNAL(timeout()), this, SLOT(OnTimerReconnectToSerial()));
}

void CTemp_Controller::InitThread()
{
    m_ptrThread_ProcessCmdbuf = new CThread_Temp_Controller_ProcessCmdbuf(this);

    if(m_ptrThread_ProcessCmdbuf)
    {
        connect(m_ptrThread_ProcessCmdbuf, SIGNAL(ReadySendData(const char*, quint64)), this, SLOT(SendDataToSerial(const char*, quint64)), Qt::BlockingQueuedConnection);
        connect(m_ptrThread_ProcessCmdbuf, SIGNAL(finished()), m_ptrThread_ProcessCmdbuf, SLOT(deleteLater()));
        m_ptrThread_ProcessCmdbuf->start();
    }

    m_ptrThread_ProcessCommbuf = new CThread_Temp_Controller_ProcessCommbuf(this, &m_Queue);

    if(m_ptrThread_ProcessCommbuf)
    {
        connect(m_ptrThread_ProcessCommbuf, SIGNAL(finished()), m_ptrThread_ProcessCommbuf, SLOT(deleteLater()));
        m_ptrThread_ProcessCommbuf->start();
    }
}

QList<QSerialPortInfo> CTemp_Controller::GetPortsInfo()
{
    const auto infos = QSerialPortInfo::availablePorts();
    return infos;
}

void CTemp_Controller::ConnectToSerial()
{
    if(m_SerialPort)
    {
        QSettings settings(m_parent->m_strName_Enterprise, m_parent->m_strName_Program);

        // Connect Serial
        int Ret = 0;
        Ret = StartSerialPort
        (
            settings.value(QString::asprintf("Config/SER/Temp_Ctrl/Port")).toString(),
            settings.value(QString::asprintf("Config/SER/Temp_Ctrl/Baudrate")).toUInt(),
            QSerialPort::Data8,
            QSerialPort::NoParity,
            QSerialPort::TwoStop,
            QSerialPort::FlowControl::NoFlowControl
        );

        if (Ret >= 0)
        {    
            m_Check_SerialPort_isOpen = 1;

            for(int i = 0; i < LOADER_CHANNEL; i++)
            {
                if(m_parent->m_ptrMonitor[i] != nullptr)
                {
                    m_parent->m_ptrMonitor[i]->SetTempControllerLED();
                }
            }

            m_pTimer_ReconnectToSerial->stop();
        }
        else
        {
            m_parent->SaveLogFile(m_parent->m_strLogFileName_Communication, "Fail to connect Temp Controller comport");
            qDebug() << "Fail to connect Temp Controller comport";
        }
    }
}

int CTemp_Controller::StartSerialPort
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

int CTemp_Controller::StopSerialPort()
{
    if(m_Check_SerialPort_isOpen)
        m_SerialPort->close();

    m_Check_SerialPort_isOpen = 0;

    for(int i = 0; i < LOADER_CHANNEL; i++)
    {
        if(m_parent->m_ptrMonitor[i] != nullptr)
        {
            m_parent->m_ptrMonitor[i]->SetTempControllerLED();
        }
    }

    return 0;
}

void CTemp_Controller::InitTempControllerSetting(quint8 index)
{
    m_STC_Comm_SendData[index].Data.DeviceID = m_parent->m_TempController_DeviceID[index];
    m_Hash_AddressToMonitor[m_parent->m_TempController_DeviceID[index]] = index;

    GetTempControllerPoint(index);
}

void CTemp_Controller::GetTempControllerPoint(quint8 index)
{
    m_STC_Comm_SendData[index].Data.StartAddr = 0x03EB;
    m_STC_Comm_SendData[index].Data.Data = 0x0001;
    m_ptrThread_ProcessCmdbuf->SetCmdStatus(index, static_cast<quint16>(CMD_TEMPCONTROLLER_REQUESTDATA::GETPOINT));
    m_Comm_Response_Check[index] = 0;
}

void CTemp_Controller::SetTempControllerPoint(quint8 index, double val)
{
    m_STC_Comm_SendData[index].Data.StartAddr = 0x0000;
    m_STC_Comm_SendData[index].Data.Data = (uint16_t)val;
    m_STC_Comm_SendData[index].Data.SV = (float)val;
    m_ptrThread_ProcessCmdbuf->SetCmdStatus(index, static_cast<quint16>(CMD_TEMPCONTROLLER_REQUESTDATA::SETPOINT));
    m_Comm_Response_Check[index] = 0;
}

void CTemp_Controller::GetTempControllerMeas(quint8 index)
{
    m_STC_Comm_SendData[index].Data.StartAddr = 0x03E8;
    m_STC_Comm_SendData[index].Data.Data = 0x0001;
    m_ptrThread_ProcessCmdbuf->SetCmdStatus(index, static_cast<quint16>(CMD_TEMPCONTROLLER_REQUESTDATA::GETMEAS));
    m_Comm_Response_Check[index] = 0;
}

void CTemp_Controller::ErrHandleTempController(quint8 index)
{
    switch(m_STC_Comm_RecvData[index].Data.Err)
    {
    case 0:     // Success
        break;
    case TEMP_CONTROLLER_ERR_ILLEGAL_FUNCTION:
        qDebug() << "지원되지 않는 명령";
        m_parent->SaveLogFile(m_parent->m_strLogFileName_Monitor + QString::asprintf("%d", index + 1), "Temp : 지원되지 않는 명령");
        break;
    case TEMP_CONTROLLER_ERR_ILLEGAL_DATA_ADDRESS:
        qDebug() << "쿼리된 데이터의 시작 주소가 장치에서 전송할 수 있는 주소와 일치하지 않습니다.";
        m_parent->SaveLogFile(m_parent->m_strLogFileName_Monitor + QString::asprintf("%d", index + 1), "Temp : 쿼리된 데이터의 시작 주소가 장치에서 전송할 수 있는 주소와 일치하지 않습니다.");
        break;
    case TEMP_CONTROLLER_ERR_ILLEGAL_DATA_VALUE:
        qDebug() << "조회된 데이터 개수가 장치에서 전송할 수 있는 데이터 개수와 일치하지 않습니다.";
        m_parent->SaveLogFile(m_parent->m_strLogFileName_Monitor + QString::asprintf("%d", index + 1), "Temp : 조회된 데이터 개수가 장치에서 전송할 수 있는 데이터 개수와 일치하지 않습니다.");
        break;
    case TEMP_CONTROLLER_ERR_SLAVE_DEVICE_FAILURE:
        qDebug() << "조회된 주문을 제대로 완료하지 못했습니다.";
        m_parent->SaveLogFile(m_parent->m_strLogFileName_Monitor + QString::asprintf("%d", index + 1), "Temp : 조회된 주문을 제대로 완료하지 못했습니다.");
        break;
    default:
        break;
    }
}

/*
    Serial slots
*/
void CTemp_Controller::RecvDataFromSerial()
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

int CTemp_Controller::SendDataToSerial(const char* data, quint64 len)
{
    if(m_SerialPort == nullptr) return SOCKET_ERROR;
    if(!m_Check_SerialPort_isOpen) return SOCKET_ERROR;
    if(m_SerialPort->bytesToWrite() > 0) m_SerialPort->clear(QSerialPort::Direction::Output);

    int Ret = SOCKET_ERROR;

    Ret = m_SerialPort->write(reinterpret_cast<const char*>(data), len);
    m_SerialPort->waitForBytesWritten(WAIT_FOR_BYTE_WRITTEN);

    return Ret;
}

void CTemp_Controller::SerialError(QSerialPort::SerialPortError error)
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

    //m_parent->SaveLogFile(m_parent->m_strLogFileName_Communication, "Temp Ctrl:" + m_SerialPort->errorString() + ":" + QString(error));
    qDebug() << m_SerialPort->errorString() << ":" << QString(error);
}

/*
    timer slots
*/
void CTemp_Controller::OnTimerReconnectToSerial()
{
    ConnectToSerial();
}
