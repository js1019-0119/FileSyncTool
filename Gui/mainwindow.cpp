#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "mainwindowuidata.h"
#include "scansettingsdialog.h"
#include "syncmodedialog.h"
#include <QMessageBox>
#include <QDebug>
#include <QPushButton>
#include <QStyle>
#include <QSettings>
#include <QLabel>
#include <QShortcut>
#include <QKeySequence>
#include <QCloseEvent>
#include <QStatusBar>
#include <QDateTime>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    initGui();
    initConnect();
    //恢复上次状态
    loadSettings();
    appendSystemLog(QStringLiteral("程序启动完成。工作目录：%1").arg(m_engine->getWorkSpace()),
                    QDateTime::currentDateTime(),SYSTEMLOGTYPE::SYSTEMLOGTYPE_INFORMATION);

    //启动后检测未完成任务，提示续传或清理
    QTimer::singleShot(0, this, &MainWindow::promptResume);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::initGui()
{
    setWindowTitle(Title_MainWindow);
    //最小窗口尺寸，保证布局不塌陷
    setMinimumSize(1024, 680);

    //调度引擎
    m_engine = new DispatchEngine(this);

    //自动扫描
    m_autoTimer = new QTimer;
    m_autoTimer->setSingleShot(true);
    m_autoTimer->setInterval(AutoScanInterval);

    //界面比例
    ui->splitter_main->setStretchFactor(0, 3);
    ui->splitter_main->setStretchFactor(1, 2);
    ui->splitter_main->setSizes(QList<int>() << 420 << 300);
    ui->splitter_bottom->setStretchFactor(0, 3);
    ui->splitter_bottom->setStretchFactor(1, 2);
    ui->splitter_bottom->setSizes(QList<int>() << 660 << 440);

    //防止面板塌陷
    ui->wd_diffrence->setMinimumHeight(220);
    ui->wd_task->setMinimumHeight(140);
    ui->wd_log->setMinimumHeight(140);

    //菜单图标
    ui->action_dir_A->setIcon(style()->standardIcon(QStyle::SP_DirIcon));
    ui->action_dir_B->setIcon(style()->standardIcon(QStyle::SP_DirIcon));
    ui->action_quit->setIcon(style()->standardIcon(QStyle::SP_DialogCloseButton));
    ui->action_scan_set->setIcon(style()->standardIcon(QStyle::SP_FileDialogDetailedView));
    ui->action_sync_set->setIcon(style()->standardIcon(QStyle::SP_BrowserReload));
    ui->action_about->setIcon(style()->standardIcon(QStyle::SP_MessageBoxInformation));

    //状态栏
    m_statusLabel = new QLabel(QStringLiteral("就绪"), this);
    ui->statusBar->addWidget(m_statusLabel, 1);

    //快捷键
    QShortcut* scanShortcut = new QShortcut(QKeySequence(QStringLiteral("F5")), this);
    connect(scanShortcut, &QShortcut::activated, this, &MainWindow::startScan_Slots);
    QShortcut* syncShortcut = new QShortcut(QKeySequence(QStringLiteral("Ctrl+Return")), this);
    connect(syncShortcut, &QShortcut::activated, this, &MainWindow::startSync_Slots);
}

void MainWindow::initConnect()
{
    //ui
    //自动扫描
    connect(m_autoTimer,&QTimer::timeout,this,&MainWindow::AutoTimerTimeout_slots);
    //设置目录
   connect(ui->wd_catalogue,&CatalogueWidget::setDirA,this,&MainWindow::setDirectoryA_Slots);
   connect(ui->wd_catalogue,&CatalogueWidget::setDirB,this,&MainWindow::setDirectoryB_Slots);
   //设置扫描参数
   connect(ui->wd_diffrence,&ScanDifftWidget::ScanOptionsChanged,this,&MainWindow::setScanOptions_Slots);
   //扫描
   connect(ui->wd_diffrence,&ScanDifftWidget::StartScan,this,&MainWindow::startScan_Slots);

   //调度引擎
   connect(m_engine,&DispatchEngine::scanProgress,this,&MainWindow::ScanProgressChanged_Slots);
   connect(m_engine,&DispatchEngine::scanFinished,this,&MainWindow::ScanFinished_Slots);
   connect(m_engine,&DispatchEngine::scanFailed,this,&MainWindow::ScanFailed_Slots);
   connect(m_engine,&DispatchEngine::MoinitorNotified,this,&MainWindow::MoinitorNotified_Slots);

   //日志
   connect(m_engine,&DispatchEngine::dispatchSystemData,ui->wd_log,&LogWidget::appendLog);

   //同步
   connect(ui->wd_diffrence,&ScanDifftWidget::StartSync,this,&MainWindow::startSync_Slots);
   connect(m_engine,&DispatchEngine::syncTaskStarted,this,&MainWindow::SyncTaskStarted_Slots);
   connect(m_engine,&DispatchEngine::syncTaskProgress,this,&MainWindow::SyncTaskProgress_Slots);
   connect(m_engine,&DispatchEngine::syncTaskStateChanged,this,&MainWindow::SyncTaskStateChanged_Slots);
   connect(m_engine,&DispatchEngine::syncTaskFinished,this,&MainWindow::SyncTaskFinished_Slots);
   connect(m_engine,&DispatchEngine::syncOverallProgress,this,&MainWindow::SyncOverallProgress_Slots);
   connect(m_engine,&DispatchEngine::syncAllFinished,this,&MainWindow::SyncAllFinished_Slots);

   //任务面板按钮
   connect(ui->wd_task,&TaskWidget::StartAllClicked,this,&MainWindow::TaskStartAll_Slots);
   connect(ui->wd_task,&TaskWidget::PauseAllClicked,this,&MainWindow::TaskPauseAll_Slots);
   connect(ui->wd_task,&TaskWidget::CancelAllClicked,this,&MainWindow::TaskCancelAll_Slots);
   //单个任务右键操作
   connect(ui->wd_task,&TaskWidget::PauseTask,this,&MainWindow::TaskPauseTask_Slots);
   connect(ui->wd_task,&TaskWidget::ResumeTask,this,&MainWindow::TaskResumeTask_Slots);
   connect(ui->wd_task,&TaskWidget::CancelTask,this,&MainWindow::TaskCancelTask_Slots);

   //菜单
   connect(ui->action_dir_A,&QAction::triggered,this,&MainWindow::menu_SelectDirA_Slots);
   connect(ui->action_dir_B,&QAction::triggered,this,&MainWindow::menu_SelectDirB_Slots);
   connect(ui->action_quit,&QAction::triggered,this,&MainWindow::menu_Quit_Slots);
   connect(ui->action_scan_set,&QAction::triggered,this,&MainWindow::menu_ScanSetting_Slots);
   connect(ui->action_sync_set,&QAction::triggered,this,&MainWindow::menu_SyncMode_Slots);
   connect(ui->action_about,&QAction::triggered,this,&MainWindow::menu_About_Slots);
   connect(ui->action_difft_List,&QAction::triggered,this,&MainWindow::menu_ToggleDiff_Slots);
   connect(ui->action_task_Win,&QAction::triggered,this,&MainWindow::menu_ToggleTask_Slots);
   connect(ui->action_log_Win,&QAction::triggered,this,&MainWindow::menu_ToggleLog_Slots);
}

void MainWindow::refreshDirectoryNames()
{
    //同步目录到差异面板
    ui->wd_diffrence->setCompareDirs(ui->wd_catalogue->getDirA(), ui->wd_catalogue->getDirB());
}

void MainWindow::AutoTimerTimeout_slots()
{
    if (m_engine->getDirA().isEmpty() || m_engine->getDirB().isEmpty())
        return;
    if (m_engine->isScanning())
        return;
    appendSystemLog(QStringLiteral("检测到目录变化，自动刷新差异列表…"), QDateTime::currentDateTime(),SYSTEMLOGTYPE::SYSTEMLOGTYPE_INFORMATION);
    m_engine->startScan();
}

void MainWindow::setDirectoryA_Slots(const QString &dir)
{
    m_engine->setCompareDirs(dir, ui->wd_catalogue->getDirB());
    refreshDirectoryNames();
}

void MainWindow::setDirectoryB_Slots(const QString &dir)
{
    m_engine->setCompareDirs(ui->wd_catalogue->getDirA(),dir);
    refreshDirectoryNames();
}

void MainWindow::appendSystemLog(const QString &msg, QDateTime time, SYSTEMLOGTYPE type)
{
    qDebug() << "日志：" << msg;
    ui->wd_log->appendLog(msg, time, type);
}

void MainWindow::setScanOptions_Slots(const ScanOptions &options)
{
    m_engine->setOptions(options);
}

void MainWindow::startScan_Slots()
{
    if (m_engine->isScanning())
    {
        //按钮回退
        ScanCancel_Slots();
        return;
    }


    if (m_engine->getDirA().isEmpty() || m_engine->getDirB().isEmpty())
    {
        QMessageBox::warning(this, QStringLiteral("提示"),
                             QStringLiteral("请先选择目录 A 和目录 B"));
        return;
    }

    ui->wd_diffrence->setbtnScanText(QStringLiteral("取消扫描"));
    ui->wd_diffrence->setScanProgressValue(0);
    m_statusLabel->setText(QStringLiteral("正在扫描…"));
    m_engine->startScan();
}

void MainWindow::ScanCancel_Slots()
{
    m_engine->cancelScan();
    ui->wd_diffrence->setbtnScanText("开始扫描");
    ui->wd_diffrence->resetScanProgress();
    m_statusLabel->setText(QStringLiteral("就绪"));
}

void MainWindow::ScanProgressChanged_Slots(int percent, const QString &text)
{
    if (percent < 0)
        ui->wd_diffrence->setScanBusy(text);          //扫描阶段：忙碌动画 + 条目数
    else
    {
        ui->wd_diffrence->setScanProgressValue(percent); //真实百分比阶段
        ui->wd_diffrence->setScanProgressText(text);
    }
    m_statusLabel->setText(text);
}

void MainWindow::ScanFinished_Slots(const DiffItemList &items)
{
    ui->wd_diffrence->setbtnScanText(QStringLiteral("开始扫描"));
    ui->wd_diffrence->setDiffItems(items);
    ui->wd_diffrence->resetScanProgress();
    m_statusLabel->setText(QStringLiteral("扫描完成，共 %1 项差异").arg(items.size()));
}

void MainWindow::ScanFailed_Slots(const QString &error)
{
    Q_UNUSED(error);
    ui->wd_diffrence->setbtnScanText(QStringLiteral("开始扫描"));
    ui->wd_diffrence->resetScanProgress();
    m_statusLabel->setText(QStringLiteral("扫描失败"));
}

void MainWindow::MoinitorNotified_Slots(const QStringList &dirs)
{
    //监控变化引擎自动刷新这里仅记录
    //废弃
    Q_UNUSED(dirs);
}

// 同步
void MainWindow::startSync_Slots()
{
    if (m_engine->isSyncing())
        return;

    //收集当前差异列表（含用户勾选/方向修改）
    const DiffItemList items = ui->wd_diffrence->getDiffItems();

    //删除确认
    int removals = 0;
    for (const FileDiffItem& item : items)
    {
        if (item.checked &&
            (item.dir == SyncSuggDirection::SyncSuggDirection_RemoveInA ||
             item.dir == SyncSuggDirection::SyncSuggDirection_RemoveInB))
            ++removals;
    }
    if (removals > 0)
    {
        const int ret = QMessageBox::question(this, QStringLiteral("确认删除"),
            QStringLiteral("本次同步将删除 %1 个文件/目录，是否继续？").arg(removals),
            QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
        if (ret != QMessageBox::Yes)
            return;
    }

    //回写勾选/方向到引擎并开始同步
    m_engine->setDifferentList(items);
    m_engine->startSync();

    //刷新任务面板
    ui->wd_task->setTasks(m_engine->getSyncTasks());
    ui->wd_task->setOverallProgress(0, 0);
    ui->wd_task->setOverallText(QStringLiteral("同步中…"));
    m_statusLabel->setText(QStringLiteral("同步中…"));
}

void MainWindow::SyncTaskStarted_Slots(int id)
{
    ui->wd_task->updateTask(id);
}

void MainWindow::SyncTaskProgress_Slots(int id, qint64 done, qint64 total)
{
    Q_UNUSED(done); Q_UNUSED(total);
    ui->wd_task->updateTask(id);
}

void MainWindow::SyncTaskStateChanged_Slots(int id, int state, const QString& error)
{
    Q_UNUSED(state); Q_UNUSED(error);
    ui->wd_task->updateTask(id);
}

void MainWindow::SyncTaskFinished_Slots(int id, int state, const QString& error)
{
    Q_UNUSED(state); Q_UNUSED(error);
    ui->wd_task->updateTask(id);
}

void MainWindow::SyncOverallProgress_Slots(qint64 done, qint64 total)
{
    ui->wd_task->setOverallProgress(done, total);
}

void MainWindow::SyncAllFinished_Slots()
{
    ui->wd_task->setOverallProgress(100, 100);
    ui->wd_task->setOverallText(QStringLiteral("同步完成"));
    m_statusLabel->setText(QStringLiteral("同步完成"));
    //同步完成后引擎会自动重扫刷新差异列表
}

void MainWindow::TaskStartAll_Slots()
{
    m_engine->resumeAllSync();
}

void MainWindow::TaskPauseAll_Slots()
{
    m_engine->pauseAllSync();
}

void MainWindow::TaskCancelAll_Slots()
{
    m_engine->cancelAllSync();
}

void MainWindow::TaskPauseTask_Slots(int id)
{
    m_engine->pauseSyncTask(id);
}

void MainWindow::TaskResumeTask_Slots(int id)
{
    m_engine->resumeSyncTask(id);
}

void MainWindow::TaskCancelTask_Slots(int id)
{
    m_engine->cancelSyncTask(id);
}

// 崩溃恢复
void MainWindow::promptResume()
{
    const QList<SyncTaskPtr> pending = m_engine->loadUnfinishedTasks();
    if (pending.isEmpty())
        return;

    QMessageBox box(this);
    box.setWindowTitle(QStringLiteral("恢复未完成传输"));
    box.setText(QStringLiteral("检测到上次有 %1 个未完成的传输任务。").arg(pending.size()));
    box.setInformativeText(QStringLiteral("是否继续上次未完成的传输？（选择\"清理\"将删除临时分片文件）"));
    QPushButton* resumeBtn = box.addButton(QStringLiteral("续传"), QMessageBox::AcceptRole);
    QPushButton* cleanBtn  = box.addButton(QStringLiteral("清理"), QMessageBox::DestructiveRole);
    box.setDefaultButton(resumeBtn);
    box.exec();

    if (box.clickedButton() == resumeBtn)
    {
        m_engine->resumeTasks(pending);
        ui->wd_task->setTasks(m_engine->getSyncTasks());
        ui->wd_task->setOverallText(QStringLiteral("续传中…"));
    }
    else if (box.clickedButton() == cleanBtn)
    {
        m_engine->clearUnfinishedTasks();
    }
}

// 菜单
void MainWindow::menu_SelectDirA_Slots()
{
    ui->wd_catalogue->browseDirA();
}

void MainWindow::menu_SelectDirB_Slots()
{
    ui->wd_catalogue->browseDirB();
}

void MainWindow::menu_Quit_Slots()
{
    close();
}

void MainWindow::menu_ScanSetting_Slots()
{
    ScanSettingsDialog dlg(m_engine->getOptions(), this);
    if (dlg.exec() == QDialog::Accepted)
    {
        const ScanOptions opt = dlg.options();
        m_engine->setOptions(opt);
        ui->wd_diffrence->applyScanOptions(opt);
        appendSystemLog(QStringLiteral("扫描设置已更新"), QDateTime::currentDateTime(), SYSTEMLOGTYPE::SYSTEMLOGTYPE_INFORMATION);
    }
}

void MainWindow::menu_SyncMode_Slots()
{
    SyncModeDialog dlg(m_engine->getOptions().mode, this);
    if (dlg.exec() == QDialog::Accepted)
    {
        ScanOptions opt = m_engine->getOptions();
        opt.mode = dlg.mode();
        m_engine->setOptions(opt);
        ui->wd_diffrence->applyScanOptions(opt);
        appendSystemLog(QStringLiteral("同步模式已更新"), QDateTime::currentDateTime(), SYSTEMLOGTYPE::SYSTEMLOGTYPE_INFORMATION);
    }
}

void MainWindow::menu_About_Slots()
{
    QMessageBox::about(this, QStringLiteral("关于"),
        QStringLiteral("<b>桌面同步工具 FileSyncTool</b><br>"
                       "基于 Qt 5.12.4 开发的双向文件同步工具。<br>"
                       "支持目录对比、并发传输、断点续传、实时监控与冲突处理。"));
}

void MainWindow::menu_ToggleDiff_Slots(bool checked)
{
    ui->wd_diffrence->setVisible(checked);
}

void MainWindow::menu_ToggleTask_Slots(bool checked)
{
    ui->wd_task->setVisible(checked);
}

void MainWindow::menu_ToggleLog_Slots(bool checked)
{
    ui->wd_log->setVisible(checked);
}

// 持久化
void MainWindow::loadSettings()
{
    QSettings settings;

    //窗口几何与分隔条位置
    restoreGeometry(settings.value(QStringLiteral("window/geometry")).toByteArray());
    ui->splitter_main->restoreState(settings.value(QStringLiteral("window/splitter_main")).toByteArray());
    ui->splitter_bottom->restoreState(settings.value(QStringLiteral("window/splitter_bottom")).toByteArray());

    //目录
    const QString dirA = settings.value(QStringLiteral("dirs/A")).toString();
    const QString dirB = settings.value(QStringLiteral("dirs/B")).toString();
    if (!dirA.isEmpty())
        ui->wd_catalogue->setDirectoryA(dirA);
    if (!dirB.isEmpty())
        ui->wd_catalogue->setDirectoryB(dirB);

    //目录一次性同步到引擎
    if (!dirA.isEmpty() || !dirB.isEmpty())
    {
        m_engine->setCompareDirs(ui->wd_catalogue->getDirA(), ui->wd_catalogue->getDirB());
        refreshDirectoryNames();
    }

    //扫描设置
    ScanOptions opt = m_engine->getOptions();
    opt.hashlogo      = settings.value(QStringLiteral("options/hashlogo"), opt.hashlogo).toString();
    opt.forceHash     = settings.value(QStringLiteral("options/forceHash"), opt.forceHash).toBool();
    opt.listSameFiles = settings.value(QStringLiteral("options/listSameFiles"), opt.listSameFiles).toBool();
    opt.mirrorDelete  = settings.value(QStringLiteral("options/mirrorDelete"), opt.mirrorDelete).toBool();
    opt.mode          = SyncMode(settings.value(QStringLiteral("options/mode"), int(opt.mode)).toInt());
    m_engine->setOptions(opt);
    ui->wd_diffrence->applyScanOptions(opt);
}

void MainWindow::saveSettings()
{
    QSettings settings;
    settings.setValue(QStringLiteral("window/geometry"), saveGeometry());
    settings.setValue(QStringLiteral("window/splitter_main"), ui->splitter_main->saveState());
    settings.setValue(QStringLiteral("window/splitter_bottom"), ui->splitter_bottom->saveState());
    settings.setValue(QStringLiteral("dirs/A"), m_engine->getDirA());
    settings.setValue(QStringLiteral("dirs/B"), m_engine->getDirB());

    const ScanOptions opt = m_engine->getOptions();
    settings.setValue(QStringLiteral("options/hashlogo"), opt.hashlogo);
    settings.setValue(QStringLiteral("options/forceHash"), opt.forceHash);
    settings.setValue(QStringLiteral("options/listSameFiles"), opt.listSameFiles);
    settings.setValue(QStringLiteral("options/mirrorDelete"), opt.mirrorDelete);
    settings.setValue(QStringLiteral("options/mode"), int(opt.mode));
}

void MainWindow::closeEvent(QCloseEvent* event)
{
    saveSettings();
    event->accept();
}
