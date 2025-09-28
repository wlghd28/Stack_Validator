#include "Stack_Validator.h"
#include "ui_Stack_Validator.h"

Stack_Validator::Stack_Validator(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::Stack_Validator)
{
    ui->setupUi(this);
    Init();
}

Stack_Validator::~Stack_Validator()
{
    delete ui;
}

/*
    Event Slots
*/
void Stack_Validator::on_pushButton_Config_clicked()
{
    CreateDialogConfig();
}

void Stack_Validator::on_pushButton_Monitor_clicked()
{
    CreateDialogMonitor(ui->comboBox_Monitor->currentIndex());
}

void Stack_Validator::on_pushButton_Exit_clicked()
{
    this->close();
}


/*
    Override Methods
*/
bool Stack_Validator::eventFilter(QObject* dest, QEvent* event)
{
    switch(event->type())
    {
    case QEvent::WindowStateChange:
        break;

    default:
        break;
    }

    return QObject::eventFilter(dest, event);
}

void Stack_Validator::resizeEvent(QResizeEvent* event)
{
   QMainWindow::resizeEvent(event);
   // Your code here.
   resize(event->size());
}

void Stack_Validator::closeEvent(QCloseEvent *event)
{
    event->accept();
    for(int i = 0; i < LOADER_CHANNEL; i++)
    {
        if(m_ptrMonitor[i]) m_ptrMonitor[i]->close();
    }
    if(m_ptrConfig) m_ptrConfig->close();
}

/*
    Methods
*/
void Stack_Validator::Init()
{
    // 빌드 날짜 프로그램 타이틀바에 표시
    setWindowTitle(QString::asprintf("%s (Menu) (ver %s %s)", m_strName_Program.toLocal8Bit().data(), __DATE__, __TIME__));

    //installEventFilter(this);

    //QThreadPool::globalInstance()->setMaxThreadCount(24);

    InitMember();
    InitWidgets();
    InitLoader();
    InitFlow_Controller();
    InitPump_Controller();
    InitTemp_Controller();
    InitMonitor();
    InitTimer();
    InitPID();
}

void Stack_Validator::InitMember()
{
    m_ptrConfig = nullptr;
    m_ptrLoader = nullptr;
    m_ptrFlow_Controller = nullptr;
    m_ptrTemp_Controller = nullptr;
    for(int i = 0; i < LOADER_CHANNEL; i++)
    {
        m_ptrMonitor[i] = nullptr;
        m_ptrPump_Controller[i] = nullptr;
        m_ptrCVM_Controller[i] = nullptr;
    }
    m_PID = nullptr;
}

void Stack_Validator::InitMonitor()
{
    QSettings settings(m_strName_Enterprise, m_strName_Program);

    for(int i = 0; i < LOADER_CHANNEL; i++)
    {
        // Active Mode
        m_Active_Mode[i] = 255;

        // E_LOAD
        m_Loader_Channel[i] = settings.value(QString::asprintf("Config/E_LOAD/Monitor%d/Channel", i + 1)).toUInt();
        m_ptrLoader->m_STC_Comm_SendData[m_Loader_Channel[i]].Conf.Curr_Range = settings.value(QString::asprintf("Config/E_LOAD/Monitor%d/CurrentRange", i + 1)).toString();
        m_ptrLoader->m_STC_Comm_SendData[m_Loader_Channel[i]].Conf.Volt_Range = settings.value(QString::asprintf("Config/E_LOAD/Monitor%d/VoltageRange", i + 1)).toString();

        // FLOW
        m_FlowController_CathodeAddr[i] = settings.value(QString::asprintf("Config/FLOW/Monitor%d/Cathode_Addr", i + 1)).toUInt();
        m_FlowController_AnodeAddr[i] = settings.value(QString::asprintf("Config/FLOW/Monitor%d/Anode_Addr", i + 1)).toUInt();

        // PUMP
        m_PumpController_Model[i] = settings.value(QString::asprintf("Config/PUMP/Monitor%d/Model", i + 1)).toUInt();

        // TEMP
        m_TempController_DeviceID[i] = settings.value(QString::asprintf("Config/TEMP/Monitor%d/Addr", i + 1)).toUInt();
    }
}

void Stack_Validator::InitWidgets()
{
    ui->comboBox_ActiveMode->addItem("Methanol");
    //ui->comboBox_ActiveMode->addItem("Hydrogen");

    for(int i = 0; i < LOADER_CHANNEL; i++)
    {
        ui->comboBox_Monitor->addItem(QString::asprintf("Monitor%d", i + 1));
    }
}

void Stack_Validator::InitTimer()
{

}

void Stack_Validator::CreateDialogConfig()
{
    if(m_ptrConfig == nullptr)
    {
        m_ptrConfig = new CConfig(this, ui->comboBox_ActiveMode->currentIndex());
        if(m_ptrConfig)
        {
            m_ptrConfig->setAttribute(Qt::WA_DeleteOnClose, true);
            m_ptrConfig->show();
        }
    }
    else
    {
        m_ptrConfig->raise();
    }
}

void Stack_Validator::CreateDialogMonitor(quint8 Monitor_Num)
{
    if(m_ptrMonitor[Monitor_Num] == nullptr)
    {
        m_ptrMonitor[Monitor_Num] = new CMonitor(this, Monitor_Num, ui->comboBox_ActiveMode->currentIndex());
        if(m_ptrMonitor[Monitor_Num])
        {
            m_ptrMonitor[Monitor_Num]->setAttribute(Qt::WA_DeleteOnClose, true);
            m_ptrMonitor[Monitor_Num]->show();
        }
    }
    else
    {
        m_ptrMonitor[Monitor_Num]->raise();
    }
}

void Stack_Validator::ClearPtrConfig()
{
    m_ptrConfig = nullptr;
}

void Stack_Validator::ClearPtrMonitor(quint8 Monitor_Num)
{
    m_ptrMonitor[Monitor_Num] = nullptr;
}

void Stack_Validator::InitLoader()
{
    // Create Loader
    m_ptrLoader = new CLoader(this);
    if(m_ptrLoader == nullptr)
    {
        QMessageBox::warning(this, windowTitle(), "Creating Loader SerialPort was failed!!");
        exit(EXIT_FAILURE);
    }
}

void Stack_Validator::InitFlow_Controller()
{
    // Create Flow_Controller
    m_ptrFlow_Controller = new CFlow_Controller(this);
    if(m_ptrFlow_Controller == nullptr)
    {
        QMessageBox::warning(this, windowTitle(), "Creating Flow_Controller SerialPort was failed!!");
        exit(EXIT_FAILURE);
    }
}

void Stack_Validator::InitPump_Controller()
{
    for(int i = 0; i < LOADER_CHANNEL; i++)
    {
        m_ptrPump_Controller[i] = new CPump_Controller(this, i);
        if(m_ptrPump_Controller[i] == nullptr)
        {
            QMessageBox::warning(this, windowTitle(), "Creating Pump_Controller SerialPort was failed!!");
            exit(EXIT_FAILURE);
        }
    }
}

void Stack_Validator::InitTemp_Controller()
{
    // Create Flow_Controller
    m_ptrTemp_Controller = new CTemp_Controller(this);
    if(m_ptrTemp_Controller == nullptr)
    {
        QMessageBox::warning(this, windowTitle(), "Creating Temp_Controller SerialPort was failed!!");
        exit(EXIT_FAILURE);
    }
}

void Stack_Validator::ReconnectToCommunication()
{
    m_ptrLoader->StopSerialPort();
    m_ptrFlow_Controller->StopSerialPort();
    m_ptrTemp_Controller->StopSerialPort();

    for(int i = 0; i < LOADER_CHANNEL; i++)
    {
        m_ptrPump_Controller[i]->StopSerialPort();
    }

    for(int i = 0; i < LOADER_CHANNEL; i++)
    {
        m_ptrCVM_Controller[i]->StopSerialPort();
    }

    m_ptrLoader->ConnectToSerial();
    m_ptrFlow_Controller->ConnectToSerial();
    m_ptrTemp_Controller->ConnectToSerial();

    for(int i = 0; i < LOADER_CHANNEL; i++)
    {
        m_ptrPump_Controller[i]->ConnectToSerial();
    }
}

void Stack_Validator::InitPID()
{
    double dt = 0.1;
    double max = 100.0;
    double min = -100.0;
    double Kp = 0.1;
    double Kd = 0.01;
    double Ki = 0.5;

    m_PID = new CPID(dt, max, min, Kp, Kd, Ki);

    if(m_PID == nullptr)
    {
        QMessageBox::warning(this, "PID Error", "Fail to init PID");
        return;
    }

    // PID 적용 알고리즘
    // double dest;   // Destination value
    // double sp;     // MFC Set Point value
    // double pv;     // Flow sensor feedback value

    // 1) Set sp to MFC
    // 2) Get pv from Flow sensor
    // 3) cal PID >> sp += m_PID_Cathode->calculate(dest, pv);
    // Repeat 1) ~ 3)
}

/*
    Timer slots
*/
