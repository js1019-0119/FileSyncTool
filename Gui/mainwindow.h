#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "dispatchengine.h"
#include <QTimer>
#include "logdata.h"

namespace Ui {
class MainWindow;
}

class QLabel;
class ScanSettingsDialog;
class SyncModeDialog;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    void initGui();
    void initConnect();
    //刷新视图目录名称
    void refreshDirectoryNames();
    //持久化
    void loadSettings();
    void saveSettings();
    void closeEvent(QCloseEvent* event) override;

protected slots:
    void AutoTimerTimeout_slots();
    void setDirectoryA_Slots(const QString &dir);
    void setDirectoryB_Slots(const QString &dir);
    void appendSystemLog(const QString &msg,QDateTime time,SYSTEMLOGTYPE type);
    void setScanOptions_Slots(const ScanOptions &options);
    //扫描
    void startScan_Slots();
    //扫描取消
    void ScanCancel_Slots();
    //扫描进度
    void ScanProgressChanged_Slots(int percent, const QString& text);
    //扫描完成
    void ScanFinished_Slots(const DiffItemList& items);
    //扫描失败
    void ScanFailed_Slots(const QString& error);
    //监控提示
    void MoinitorNotified_Slots(const QStringList& dirs);

    //同步
    void startSync_Slots();
    void SyncTaskStarted_Slots(int id);
    void SyncTaskProgress_Slots(int id, qint64 done, qint64 total);
    void SyncTaskStateChanged_Slots(int id, int state, const QString& error);
    void SyncTaskFinished_Slots(int id, int state, const QString& error);
    void SyncOverallProgress_Slots(qint64 done, qint64 total);
    void SyncAllFinished_Slots();
    void TaskStartAll_Slots();
    void TaskPauseAll_Slots();
    void TaskCancelAll_Slots();
    void TaskPauseTask_Slots(int id);
    void TaskResumeTask_Slots(int id);
    void TaskCancelTask_Slots(int id);
    //崩溃恢复
    void promptResume();

    //菜单
    void menu_SelectDirA_Slots();
    void menu_SelectDirB_Slots();
    void menu_Quit_Slots();
    void menu_ScanSetting_Slots();
    void menu_SyncMode_Slots();
    void menu_About_Slots();
    void menu_ToggleDiff_Slots(bool checked);
    void menu_ToggleTask_Slots(bool checked);
    void menu_ToggleLog_Slots(bool checked);

private:
    Ui::MainWindow *ui;

    //调度引擎
    DispatchEngine* m_engine = nullptr;

    //自动扫描
    QTimer* m_autoTimer = nullptr;

    //状态栏
    QLabel* m_statusLabel = nullptr;
};

#endif // MAINWINDOW_H
