#include "dispatchengine.h"
#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QDateTime>
#include "dispatchdata.h"
#include "synctaskdata.h"
#include "taskstatestore.h"
#include <QJsonParseError>
#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonArray>
#include <QCoreApplication>


//基线路径处理
namespace
{
    QString baselineFilePath(const QString& workspace)
    {
        return QDir(workspace).absoluteFilePath(QStringLiteral(BASELINE_FILENAME));
    }
}


DispatchEngine::DispatchEngine(QObject *parent) : QObject(parent)
{
    initObj();
}

DispatchEngine::~DispatchEngine()
{
    //保存基线
    saveBaseline();

    //停止定时器
    if(m_saveTaskTimer)
        m_saveTaskTimer->stop();
    if(m_moinitorRefreshTimer)
        m_moinitorRefreshTimer->stop();

    //停止监控并回收（析构内部会中断后台收集线程）
    if(m_moinitor)
    {
        delete m_moinitor;
        m_moinitor = nullptr;
    }

    //停止同步调度器（安全回收所有传输线程）
    if(m_scheduler)
        m_scheduler->stop();

    //扫描线程
    //断开扫描引擎连接
    if(m_scanWorker)
        QObject::disconnect(m_scanWorker,nullptr,this,nullptr);
    m_scanCancel.store(true);
    if (m_scanWorker)
    {
        //执行一次cancel
        QMetaObject::invokeMethod(m_scanWorker, "cancel", Qt::QueuedConnection);
    }
    if (m_scanThread)
    {
        m_scanThread->quit();
        if (!m_scanThread->wait(5000))
        {
            m_scanThread->terminate();
            m_scanThread->wait(2000);
        }
    }
    if (m_scanWorker)
    {
        delete m_scanWorker;
        m_scanWorker = nullptr;
    }
    if (m_scanThread)
    {
        delete m_scanThread;
        m_scanThread = nullptr;
    }
}

void DispatchEngine::setCompareDirs(const QString &dir_A, const QString &dir_B)
{
    const QString ca = dir_A.isEmpty() ? QString() : QDir::cleanPath(QFileInfo(dir_A).absoluteFilePath());
    const QString cb = dir_B.isEmpty() ? QString() : QDir::cleanPath(QFileInfo(dir_B).absoluteFilePath());
    if (ca == m_dir_A && cb == m_dir_B)
        return;

    m_dir_A = ca;
    m_dir_B = cb;

    m_diffitemList.clear();
    m_hashA.clear();
    m_hashB.clear();

    rebuildMoinitor();
    emit differentChanged();
    emit dispatchSystemData(QString("设置同步目录A：%1，目录B：%2").arg(m_dir_A).arg(m_dir_B),
                            QDateTime::currentDateTime(),SYSTEMLOGTYPE::SYSTEMLOGTYPE_INFORMATION);
}

void DispatchEngine::setOptions(const ScanOptions &option)
{
    m_option = option;
}

void DispatchEngine::setWorkSpace(const QString &dir)
{
    m_workspace = dir;
    QDir().mkdir(m_workspace);
}

void DispatchEngine::startScan()
{
    if (m_scanning)
        return;

    if (m_dir_A.isEmpty() || m_dir_B.isEmpty())
    {
        emit scanFailed(QStringLiteral("请先选择源目录 A 和目标目录 B"));
        return;
    }
    if (m_dir_A == m_dir_B)
    {
        emit scanFailed(QStringLiteral("目录 A 与目录 B 不能相同"));
        return;
    }

    m_scanning = true;
    m_scanCancel.store(false);

    const ScanOptions opt = m_option;
    const QString a = m_dir_A;
    const QString b = m_dir_B;
    const BaselineHash base = m_baseline;
    ScanWorker* w = m_scanWorker;

    emit dispatchSystemData(QStringLiteral("开始扫描：A=%1  B=%2").arg(a, b),QDateTime::currentDateTime(),SYSTEMLOGTYPE::SYSTEMLOGTYPE_INFORMATION);

    //跨线程调用扫描
    QMetaObject::invokeMethod(w, [w, a, b, opt, base]()
    {
        w->scanAll(a, b, opt, base);
    }, Qt::QueuedConnection);
}

void DispatchEngine::cancelScan()
{
    if (!m_scanning)
        return;
    m_scanCancel.store(true);
    ScanWorker* w = m_scanWorker;
    QMetaObject::invokeMethod(w, [w]()
    {
        w->cancel();
    }, Qt::QueuedConnection);
}

// 同步
void DispatchEngine::startSync()
{
    if (m_syncing)
        return;

    //构建同步任务
    QList<SyncTaskPtr> tasks;

    for (const FileDiffItem& item : m_diffitemList)
    {
        //只同步勾选且有方向的
        if (!item.checked || item.dir == SyncSuggDirection::SyncSuggDirection_None)
            continue;

        const bool isDir = item.meta_A.isDir || item.meta_B.isDir;
        const QString pathA = QDir(m_dir_A).filePath(item.relPath);
        const QString pathB = QDir(m_dir_B).filePath(item.relPath);

        switch (item.dir)
        {
        case SyncSuggDirection::SyncSuggDirection_AtoB:
        {
            SyncTaskPtr t(new SyncTask);
            t->sourcePath = pathA;
            t->targetPath = pathB;
            t->relPath    = item.relPath;
            t->dir        = item.dir;
            t->isDir      = isDir;
            t->totalSize  = isDir ? 0 : item.meta_A.size;
            tasks.append(t);
            break;
        }
        case SyncSuggDirection::SyncSuggDirection_BtoA:
        {
            SyncTaskPtr t(new SyncTask);
            t->sourcePath = pathB;
            t->targetPath = pathA;
            t->relPath    = item.relPath;
            t->dir        = item.dir;
            t->isDir      = isDir;
            t->totalSize  = isDir ? 0 : item.meta_B.size;
            tasks.append(t);
            break;
        }
        case SyncSuggDirection::SyncSuggDirection_RemoveInB:
        case SyncSuggDirection::SyncSuggDirection_RemoveInA:
        {
            //镜像删除：立即执行
            const QString p = (item.dir == SyncSuggDirection::SyncSuggDirection_RemoveInB) ? pathB : pathA;
            const QFileInfo fi(p);
            bool ok = false;
            if (fi.isDir())
                ok = QDir(p).removeRecursively();
            else
                ok = QFile::remove(p);

            emit dispatchSystemData(QStringLiteral("删除：%1 %2")
                                    .arg(p, ok ? QStringLiteral("成功") : QStringLiteral("失败")),
                                    QDateTime::currentDateTime(),
                                    ok ? SYSTEMLOGTYPE::SYSTEMLOGTYPE_INFORMATION : SYSTEMLOGTYPE::SYSTEMLOGTYPE_ERRO);
            if (ok)
            {
                //同步移除基线
                m_baseline.remove(item.relPath);
                m_baseline.remove(item.relPath + QLatin1Char('/'));
            }
            break;
        }
        default:
            break;
        }
    }

    if (tasks.isEmpty())
    {
        if (m_syncing == false)
        {
            saveBaseline();
            emit dispatchSystemData(QStringLiteral("没有可同步的任务"), QDateTime::currentDateTime(), SYSTEMLOGTYPE::SYSTEMLOGTYPE_WARRING);
        }
        return;
    }

    //入队
    m_syncTasks.clear();
    for (const SyncTaskPtr& t : tasks)
    {
        m_syncTasks.append(t);
        m_scheduler->enqueue(t);
    }
    //批量入队后统一持久化一次
    m_scheduler->persistTasks(true);

    m_syncing = true;
    emit dispatchSystemData(QStringLiteral("开始同步，共 %1 个任务").arg(tasks.size()),
                            QDateTime::currentDateTime(), SYSTEMLOGTYPE::SYSTEMLOGTYPE_INFORMATION);
}

void DispatchEngine::pauseAllSync()
{
    if (m_scheduler)
        m_scheduler->pauseAll();
}

void DispatchEngine::resumeAllSync()
{
    if (m_scheduler)
        m_scheduler->resumeAll();
}

void DispatchEngine::cancelAllSync()
{
    if (m_scheduler)
        m_scheduler->cancelAll();
}

void DispatchEngine::pauseSyncTask(int id)
{
    if (m_scheduler)
        m_scheduler->pauseTask(id);
}

void DispatchEngine::resumeSyncTask(int id)
{
    if (m_scheduler)
        m_scheduler->resumeTask(id);
}

void DispatchEngine::cancelSyncTask(int id)
{
    if (m_scheduler)
        m_scheduler->cancelTask(id);
}

QList<SyncTaskPtr> DispatchEngine::loadUnfinishedTasks() const
{
    return TaskStateStore::load(TaskStateStore::defaultPath(m_workspace));
}

void DispatchEngine::resumeTasks(const QList<SyncTaskPtr>& tasks)
{
    m_syncTasks.clear();
    for (const SyncTaskPtr& t : tasks)
    {
        m_syncTasks.append(t);
        m_scheduler->enqueue(t);
    }
    m_scheduler->persistTasks(true);
    m_syncing = true;
    emit dispatchSystemData(QStringLiteral("恢复 %1 个未完成任务").arg(tasks.size()),
                            QDateTime::currentDateTime(), SYSTEMLOGTYPE::SYSTEMLOGTYPE_INFORMATION);
}

void DispatchEngine::clearUnfinishedTasks()
{
    const QList<SyncTaskPtr> tasks = loadUnfinishedTasks();
    for (const SyncTaskPtr& t : tasks)
        QFile::remove(t->tempPath());
    TaskStateStore::clear(TaskStateStore::defaultPath(m_workspace));
    emit dispatchSystemData(QStringLiteral("已清理未完成任务"), QDateTime::currentDateTime(), SYSTEMLOGTYPE::SYSTEMLOGTYPE_INFORMATION);
}

// 初始化
void DispatchEngine::initObj()
{
    m_scanCancel.store(false);

    //baseline.json,taskstate.json存放路径
    //m_workspace = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    //if (m_workspace.isEmpty())
    //    m_workspace = QDir::homePath() + QLatin1String(WorkSpanceLatin.toLatin1());
    //QDir().mkpath(m_workspace);

    m_workspace = QCoreApplication::applicationDirPath();
    if (!QFileInfo(m_workspace).isWritable())          // 运行目录不可写（如 Program Files）
        m_workspace = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (m_workspace.isEmpty())
        m_workspace = QDir::homePath() + QLatin1String(WorkSpanceLatin.toLatin1());
    QDir().mkpath(m_workspace);

    //扫描线程
    m_scanThread = new QThread();
    m_scanThread->setObjectName(QStringLiteral(SCANTHREAD_OBJNAME));

    m_scanWorker = new ScanWorker;
    m_scanWorker->moveToThread(m_scanThread);

    connect(m_scanWorker,&ScanWorker::progress,this,&DispatchEngine::scanProgress);
    connect(m_scanWorker,&ScanWorker::finished,this,&DispatchEngine::ScanFinished_Slots);
    connect(m_scanWorker,&ScanWorker::failed,this,&DispatchEngine::ScanFailed_Slots);
    connect(m_scanWorker,&ScanWorker::sendSystemLogMsg,this,&DispatchEngine::dispatchSystemData);

    m_scanThread->start();

    //同步调度器
    m_scheduler = new TaskScheduler(SyncMaxThreads, this);
    m_scheduler->setStateFilePath(TaskStateStore::defaultPath(m_workspace));
    connect(m_scheduler,&TaskScheduler::taskStarted,this,&DispatchEngine::syncTaskStarted);
    connect(m_scheduler,&TaskScheduler::taskProgress,this,&DispatchEngine::syncTaskProgress);
    connect(m_scheduler,&TaskScheduler::taskStateChanged,this,&DispatchEngine::syncTaskStateChanged);
    connect(m_scheduler,&TaskScheduler::taskFinished,this,&DispatchEngine::SyncTaskFinished_Slots);
    connect(m_scheduler,&TaskScheduler::overallProgress,this,&DispatchEngine::syncOverallProgress);
    connect(m_scheduler,&TaskScheduler::allFinished,this,&DispatchEngine::SyncAllFinished_Slots);
    m_scheduler->start();

    //监控
    m_moinitor = new FileMoinitor;
    connect(m_moinitor,&FileMoinitor::directoryChanged,this,&DispatchEngine::FileMoinitorDirChanged_slots);
    connect(m_moinitor,&FileMoinitor::sendSystemLogMsg,this,&DispatchEngine::dispatchSystemData);

    m_moinitorRefreshTimer = new QTimer;
    m_moinitorRefreshTimer->setSingleShot(true);
    m_moinitorRefreshTimer->setInterval(MoinitorRefreshInterval);
    connect(m_moinitorRefreshTimer,&QTimer::timeout,this,&DispatchEngine::MoinitorRefreshTimeout_Slots);

    //周期性保存任务状态
    m_saveTaskTimer = new QTimer;
    m_saveTaskTimer->setInterval(SaveTaskStateInterval);
    connect(m_saveTaskTimer,&QTimer::timeout,this,&DispatchEngine::SaveTaskStateTimeout_slots);
    m_saveTaskTimer->start();

    //延迟到事件循环空闲时加载，避免阻塞窗口
    QTimer::singleShot(0, this, [this]() { loadBaseline(); });
}

void DispatchEngine::loadBaseline()
{
    m_baseline.clear();
    QFile f(baselineFilePath(m_workspace));
    if (!f.exists() || !f.open(QIODevice::ReadOnly))
        return;

    QJsonParseError err;
    const QJsonDocument doc = QJsonDocument::fromJson(f.readAll(), &err);
    f.close();
    if (err.error != QJsonParseError::NoError || !doc.isObject())
        return;

    const QJsonObject root = doc.object();
    const QJsonObject files = root.value(BaseLineFile).toObject();
    for (QJsonObject::const_iterator it = files.constBegin(); it != files.constEnd(); ++it)
    {
        const QJsonObject o = it.value().toObject();
        BaselineEntry e;
        e.size  = qint64(o.value(BaseLineSize).toDouble());
        e.mtime = qint64(o.value(BaseLineTime).toDouble());
        e.hash  = o.value(BaseLineHash).toString();
        m_baseline.insert(it.key(), e);
    }
    emit dispatchSystemData(QStringLiteral("已加载基线记录 %1 条").arg(m_baseline.size()),
                            QDateTime::currentDateTime(),SYSTEMLOGTYPE::SYSTEMLOGTYPE_INFORMATION);
}

void DispatchEngine::saveBaseline()
{
    QJsonObject files;
    for (BaselineHash::const_iterator it = m_baseline.constBegin();
         it != m_baseline.constEnd(); ++it)
    {
        QJsonObject o;
        o.insert(BaseLineSize,  double(it.value().size));
        o.insert(BaseLineTime, double(it.value().mtime));
        o.insert(BaseLineHash,  it.value().hash);
        files.insert(it.key(), o);
    }

    QJsonObject root;
    root.insert(BaseLineVersion, 1);
    root.insert(BaseLineFile,   files);

    QFile f(baselineFilePath(m_workspace));
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate))
        return;
    f.write(QJsonDocument(root).toJson(QJsonDocument::Compact));
    f.close();
}

void DispatchEngine::resetBaseline()
{
    m_baseline.clear();
    saveBaseline();
    emit dispatchSystemData(QStringLiteral("基线已清空"),QDateTime::currentDateTime(),SYSTEMLOGTYPE::SYSTEMLOGTYPE_INFORMATION);
}

void DispatchEngine::rebuildMoinitor()
{
    QStringList roots;
    if (!m_dir_A.isEmpty() && QFileInfo(m_dir_A).isDir())
        roots << m_dir_A;
    if (!m_dir_B.isEmpty() && QFileInfo(m_dir_B).isDir())
        roots << m_dir_B;

    if (roots.isEmpty())
    {
        m_moinitor->stop();
        return;
    }
    m_moinitor->setRoots(roots);
    m_moinitor->start();
}

// 扫描回调
void DispatchEngine::ScanFinished_Slots(const DiffItemList &items, const FileMetaHash &mapA, const FileMetaHash &mapB)
{
    m_scanning = false;
    m_diffitemList = items;
    m_hashA  = mapA;
    m_hashB  = mapB;

    emit scanFinished(m_diffitemList);
    emit differentChanged();
}

void DispatchEngine::ScanFailed_Slots(const QString &error)
{
    m_scanning = false;
    emit  dispatchSystemData(QStringLiteral("扫描失败：%1").arg(error),QDateTime::currentDateTime(),SYSTEMLOGTYPE::SYSTEMLOGTYPE_ERRO);
    emit scanFailed(error);
}

void DispatchEngine::FileMoinitorDirChanged_slots(const QStringList& dirs)
{
    emit MoinitorNotified(dirs);

    if (m_scanning)
        return;
    if (m_moinitorRefreshTimer->isActive())
        m_moinitorRefreshTimer->stop();
    m_moinitorRefreshTimer->start();
}

void DispatchEngine::MoinitorRefreshTimeout_Slots()
{
    //监控防抖后自动重扫（同步过程中不重扫）
    if (m_scanning || m_syncing)
        return;
    startScan();
}

void DispatchEngine::SaveTaskStateTimeout_slots()
{
    //周期性持久化未完成任务
    if (m_scheduler)
        m_scheduler->persistTasks(true);
}

// 同步回调
void DispatchEngine::SyncTaskFinished_Slots(int id, int state, const QString& error)
{
    const TaskState st = static_cast<TaskState>(state);
    const SyncTaskPtr t = m_scheduler->taskById(id);

    //同步完成：更新基线，供双向同步的冲突判定
    if (st == TaskState::TaskState_Completed)
    {
        if (!t.isNull() && !t->isDir)
        {
            const QFileInfo fi(t->sourcePath);
            if (fi.exists())
            {
                BaselineEntry e;
                e.size  = fi.size();
                e.mtime = fi.lastModified().toMSecsSinceEpoch();
                m_baseline.insert(t->relPath, e);
            }
        }
    }
    else if (st == TaskState::TaskState_Failed)
    {
        //失败原因写入日志，便于排查
        const QString rel = t.isNull() ? QString::number(id) : t->relPath;
        emit dispatchSystemData(QStringLiteral("同步失败：%1 — %2").arg(rel, error),
                                QDateTime::currentDateTime(), SYSTEMLOGTYPE::SYSTEMLOGTYPE_ERRO);
    }

    emit syncTaskFinished(id, state, error);
}

void DispatchEngine::SyncAllFinished_Slots()
{
    m_syncing = false;
    saveBaseline();
    emit dispatchSystemData(QStringLiteral("同步完成"), QDateTime::currentDateTime(), SYSTEMLOGTYPE::SYSTEMLOGTYPE_INFORMATION);
    emit syncAllFinished();

    //同步完成后自动重扫，刷新差异列表
    startScan();
}
