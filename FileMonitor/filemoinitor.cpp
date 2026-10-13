#include "filemoinitor.h"
#include <QFileInfo>
#include <QDir>
#include <QDirIterator>
#include <QMutexLocker>

// DirCollector：后台目录收集
DirCollector::DirCollector(QObject *parent) : QThread(parent)
{
}

void DirCollector::startCollect(const QStringList &roots, int max)
{
    {
        QMutexLocker locker(&m_mutex);
        m_roots = roots;
        m_max = max;
    }
    m_stop.store(false);
    start();
}

void DirCollector::requestStop()
{
    m_stop.store(true);
}

void DirCollector::run()
{
    QStringList all;

    //拷贝参数，避免与主线程竞争
    QStringList roots;
    int max = 4000;
    {
        QMutexLocker locker(&m_mutex);
        roots = m_roots;
        max = m_max;
    }

    for (const QString &root : roots)
    {
        if (m_stop.load())
            break;

        QFileInfo ri(root);
        if (!ri.exists() || !ri.isDir())
            continue;

        const QString cleanRoot = QDir::cleanPath(ri.absoluteFilePath());
        if (!all.contains(cleanRoot))
            all << cleanRoot;

        QDirIterator it(root, QDir::Dirs | QDir::NoDotAndDotDot | QDir::Hidden | QDir::NoSymLinks,
                        QDirIterator::Subdirectories);
        while (it.hasNext())
        {
            if (m_stop.load())
                break;
            if (all.size() >= max)
                break;
            it.next();
            const QString p = QDir::cleanPath(it.filePath());
            if (!all.contains(p))
                all << p;
        }

        if (all.size() >= max)
            break;
    }

    if (!m_stop.load())
        emit collected(all);
}


// FileMoinitor
FileMoinitor::FileMoinitor(QObject *parent) : QObject(parent)
{
    initObj();
}

FileMoinitor::~FileMoinitor()
{
    stop();
}

void FileMoinitor::setRoots(const QStringList &roots)
{
    m_roots = roots;
    if (m_running)
    {
        stop();
        start();
    }
}

void FileMoinitor::start()
{
    if (m_running)
        return;

    m_running = true;
    m_watch_set.clear();
    m_pending_set.clear();
    m_loggedOnce = false;

    if (!m_roots.isEmpty())
        m_watcher->removePaths(m_watcher->directories());

    //后台遍历收集目录，完成后批量 addPath（不阻塞主线程）
    scheduleCollect();
}

void FileMoinitor::stop()
{
    if (!m_running)
        return;
    m_running = false;

    //中断后台收集，等待线程快速退出
    if (m_collector && m_collector->isRunning())
    {
        m_collector->requestStop();
        m_collector->wait();
    }

    m_time->stop();
    m_pending_set.clear();

    if (m_watcher)
    {
        const QStringList dirs = m_watcher->directories();
        if (!dirs.isEmpty())
            m_watcher->removePaths(dirs);
    }
    m_watch_set.clear();

    emit FileMonitorStateMsg(QStringLiteral("监控已停止"));
    emit sendSystemLogMsg(QStringLiteral("文件监控已停止"),
                          QDateTime::currentDateTime(),SYSTEMLOGTYPE::SYSTEMLOGTYPE_INFORMATION);
}

void FileMoinitor::MoinitorDirChanged_slots(const QString &p_dir)
{
    if (!m_running)
        return;

    //变化去重
    const QString clean = QDir::cleanPath(p_dir);
    m_pending_set.insert(clean);

    //增量刷新在 TimeOutFlush 中执行
    if (!m_time->isActive())
        m_time->start();
}

void FileMoinitor::TimeOutFlush_slots()
{
    if (m_pending_set.isEmpty())
        return;

    QStringList dirs = m_pending_set.toList();
    m_pending_set.clear();
    dirs.sort();

    emit directoryChanged(dirs);

    //增量刷新监控目录
    scheduleCollect();
}

void FileMoinitor::onCollected(const QStringList &dirs)
{
    if (!m_running)
        return;

    addPathsIncremental(dirs);

    emit FileMonitorStateMsg(QStringLiteral("监控目录 %1 个").arg(m_watch_set.size()));

    if (!m_loggedOnce)
    {
        m_loggedOnce = true;
        emit sendSystemLogMsg(QStringLiteral("监控已启动，共监控 %1 个目录").arg(m_watch_set.size()),
                              QDateTime::currentDateTime(),SYSTEMLOGTYPE::SYSTEMLOGTYPE_INFORMATION);
    }
}

void FileMoinitor::initObj()
{
    m_watcher = new QFileSystemWatcher(this);
    m_time = new QTimer(this);
    //单次触发
    m_time->setSingleShot(true);
    m_time->setInterval(m_time_interval);

    //后台目录收集线程
    m_collector = new DirCollector(this);

    connect(m_watcher, &QFileSystemWatcher::directoryChanged,this, &FileMoinitor::MoinitorDirChanged_slots);
    connect(m_time, &QTimer::timeout, this, &FileMoinitor::TimeOutFlush_slots);
    connect(m_collector, &DirCollector::collected, this, &FileMoinitor::onCollected);
}

void FileMoinitor::scheduleCollect()
{
    if (m_roots.isEmpty())
    {
        emit FileMonitorStateMsg(QStringLiteral("监控已启动，共监控 0 个目录"));
        if (!m_loggedOnce)
        {
            m_loggedOnce = true;
            emit sendSystemLogMsg(QStringLiteral("监控已启动，共监控 0 个目录"),
                                  QDateTime::currentDateTime(),SYSTEMLOGTYPE::SYSTEMLOGTYPE_INFORMATION);
        }
        return;
    }

    //若上一次收集仍在进行，先中断再发起新收集
    if (m_collector->isRunning())
    {
        m_collector->requestStop();
        m_collector->wait();
    }

    m_collector->startCollect(m_roots, m_max_monitorSize);
}

void FileMoinitor::addPathsIncremental(const QStringList &dirs)
{
    for (const QString &p : dirs)
    {
        if (m_watch_set.contains(p))
            continue;
        if (m_watcher->addPath(p))
            m_watch_set.insert(p);
    }
}
