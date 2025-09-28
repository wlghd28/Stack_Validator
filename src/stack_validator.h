#ifndef Stack_Validator_H
#define Stack_Validator_H

#include <QMainWindow>
#include <QtGui>
#include <QDesktopWidget>
#include <QTimer>
#include <QThread>

#include "cconfig.h"
#include "cmonitor.h"

#include "cloader.h"
#include "cflow_controller.h"
#include "cpump_controller.h"
#include "ctemp_controller.h"
#include "cpid.h"

QT_BEGIN_NAMESPACE
namespace Ui { class Stack_Validator; }
QT_END_NAMESPACE

class Stack_Validator : public QMainWindow
{
    Q_OBJECT

public:
    Stack_Validator(QWidget *parent = nullptr);
    ~Stack_Validator();

private slots:
    void on_pushButton_Config_clicked();
    void on_pushButton_Monitor_clicked();
    void on_pushButton_Exit_clicked();

private slots:

private:    // override methods
    bool eventFilter(QObject* dest, QEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void closeEvent(QCloseEvent *event) override;

private:
    Ui::Stack_Validator *ui;

public: // member
    // Communication
    CLoader* m_ptrLoader;
    CFlow_Controller* m_ptrFlow_Controller;
    CPump_Controller* m_ptrPump_Controller[LOADER_CHANNEL];
    CTemp_Controller* m_ptrTemp_Controller;

    // Monitor(1~5)
    quint8 m_Active_Mode[LOADER_CHANNEL];
    quint8 m_Loader_Channel[LOADER_CHANNEL];
    quint8 m_FlowController_CathodeAddr[LOADER_CHANNEL];
    quint8 m_FlowController_AnodeAddr[LOADER_CHANNEL];
    quint8 m_PumpController_Model[LOADER_CHANNEL];
    quint8 m_TempController_DeviceID[LOADER_CHANNEL];

    // PID Control
    CPID* m_PID;

public: // method
    void Init();
    void InitMember();
    void InitMonitor();
    void InitWidgets();
    void InitTimer();

    // Dialog
    void CreateDialogConfig();
    void CreateDialogMonitor(quint8 Monitor_Num);
    void ClearPtrConfig();
    void ClearPtrMonitor(quint8 Monitor_Num);

    // Communication
    void InitLoader();
    void InitFlow_Controller();
    void InitPump_Controller();
    void InitTemp_Controller();
    void ReconnectToCommunication();

    // PID Control
    void InitPID();
};
#endif // Stack_Validator_H
