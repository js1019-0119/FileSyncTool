#include "taskscheduler.h"
#include "taskworker.h"
#include "taskstatestore.h"
#include <QMutexLocker>
#include <QDateTime>

namespace
{
    //持久化节流阈值
    const qint64 kPersistThrottleMs = 500;
}

TaskScheduler::TaskScheduler(int maxThreads, QObject* parent)
    : QObject(parent), m_maxThreads(maxThreads)
{
}

TaskScheduler::~TaskScheduler()
{
    stop();
}

void TaskScheduler::start()
{
    if (!m_workers.isEmpty())
        return;
    for (int i = 0; i < m_maxThreads; ++i)
    {
        TransferWorker* w = new TransferWorker(this, i);
        m_workers.append(w);
        w->start();
    }
}

void TaskScheduler::stop()
{
    if (m_workers.isEmpty())
        return;
    m_stopping.store(true);
    {
        QMutexLocker locker(&m_queueMutex);
        m_queueCond.wakeAll();
    }
    for (TransferWorker* w : m_workers)
        w->requestStop();
    for (TransferWorker* w : m_workers)
    {
        w->wait();
        delete w;
    }
    m_workers.clear();
    m_stopping.store(false);
    persistTasks(true);   //停机时强制持久化一次
}

void TaskScheduler::enqueue(const SyncTaskPtr& task)
{
    task->id = m_nextId++;
    task->state = TaskState::TaskState_Pending;
    {
        QMutexLocker locker(&m_stateMutex);
        m_tasks.insert(task->id, task);
        m_totalBytes += task->totalSize;
        m_enqueued++;
    }
    {
        QMutexLocker locker(&m_queueMutex);
        m_queue.push(task);
        m_queueCond.wakeOne();
    }
}

SyncTaskPtr TaskScheduler::takeNext()
{
    QMutexLocker locker(&m_queueMutex);
    while (m_queue.empty() && !m_stopping.load())
        m_queueCond.wait(&m_queueMutex);
    if (m_queue.empty())
        return SyncTaskPtr();
    SyncTaskPtr t = m_queue.top();
    m_queue.pop();
    return t;
}

void TaskScheduler::onTaskStarted(TransferWorker* w, const SyncTaskPtr& t)
{
    {
        QMutexLocker locker(&m_stateMutex);
        m_taskWorker.insert(t->id, w);
    }
    emit taskStarted(t->id);
}

void TaskScheduler::onTaskProgress(const SyncTaskPtr& t, qint64 done, qint64 total)
{
    QMutexLocker locker(&m_stateMutex);
    const qint64 old = m_lastDone.value(t->id, 0);
    m_doneBytes += (done - old);
    m_lastDone.insert(t->id, done);
    emit taskProgress(t->id, done, total);
    emit overallProgress(m_doneBytes, m_totalBytes);
}

void TaskScheduler::onTaskStateChanged(const SyncTaskPtr& t)
{
    emit taskStateChanged(t->id, int(t->state), t->error);
}

void TaskScheduler::onTaskFinished(TransferWorker* w, const SyncTaskPtr& t, TaskState st, const QString& err)
{
    Q_UNUSED(w);
    {
        QMutexLocker locker(&m_stateMutex);
        if (st == TaskState::TaskState_Completed)
        {
            const qint64 old = m_lastDone.value(t->id, 0);
            m_doneBytes += (t->totalSize - old);
            m_lastDone.insert(t->id, t->totalSize);
        }
        else if (st == TaskState::TaskState_Canceled || st == TaskState::TaskState_Failed)
        {
            const qint64 old = m_lastDone.value(t->id, 0);
            m_doneBytes -= old;
            m_lastDone.remove(t->id);
        }
        m_taskWorker.remove(t->id);
        m_finished++;
    }
    t->state = st;
    t->error = err;
    emit taskFinished(t->id, int(st), err);
    persistTasks();
    checkAllFinished();
}

void TaskScheduler::pauseTask(int id)
{
    SyncTaskPtr t = taskById(id);
    if (t.isNull())
        return;
    if (t->state == TaskState::TaskState_Running || t->state == TaskState::TaskState_Pending)
    {
        t->pauseFlag.store(true);
        TransferWorker* w = nullptr;
        {
            QMutexLocker locker(&m_stateMutex);
            w = m_taskWorker.value(id, nullptr);
        }
        if (w)
            w->signalWake();
    }
}

void TaskScheduler::resumeTask(int id)
{
    SyncTaskPtr t = taskById(id);
    if (t.isNull())
        return;
    t->pauseFlag.store(false);
    TransferWorker* w = nullptr;
    {
        QMutexLocker locker(&m_stateMutex);
        w = m_taskWorker.value(id, nullptr);
    }
    if (w)
        w->signalWake();
}

void TaskScheduler::cancelTask(int id)
{
    SyncTaskPtr t = taskById(id);
    if (t.isNull())
        return;
    if (!SyncTask::isFinished(t->state))
    {
        t->cancelFlag.store(true);
        t->pauseFlag.store(false);   //若在暂停中，取消优先
        TransferWorker* w = nullptr;
        {
            QMutexLocker locker(&m_stateMutex);
            w = m_taskWorker.value(id, nullptr);
        }
        if (w)
            w->signalWake();
    }
}

void TaskScheduler::pauseAll()
{
    const QList<SyncTaskPtr> list = allTasks();
    for (const SyncTaskPtr& t : list)
    {
        if (t->state == TaskState::TaskState_Running || t->state == TaskState::TaskState_Pending)
            t->pauseFlag.store(true);
    }
    for (TransferWorker* w : m_workers)
        w->signalWake();
}

void TaskScheduler::resumeAll()
{
    const QList<SyncTaskPtr> list = allTasks();
    for (const SyncTaskPtr& t : list)
        t->pauseFlag.store(false);
    for (TransferWorker* w : m_workers)
        w->signalWake();
}

void TaskScheduler::cancelAll()
{
    const QList<SyncTaskPtr> list = allTasks();
    for (const SyncTaskPtr& t : list)
    {
        if (!SyncTask::isFinished(t->state))
        {
            t->cancelFlag.store(true);
            t->pauseFlag.store(false);
        }
    }
    for (TransferWorker* w : m_workers)
        w->signalWake();
}

SyncTaskPtr TaskScheduler::taskById(int id) const
{
    QMutexLocker locker(&m_stateMutex);
    return m_tasks.value(id);
}

QList<SyncTaskPtr> TaskScheduler::allTasks() const
{
    QMutexLocker locker(&m_stateMutex);
    return m_tasks.values();
}

void TaskScheduler::setStateFilePath(const QString& path)
{
    m_stateFile = path;
}

void TaskScheduler::persistTasks(bool force)
{
    if (m_stateFile.isEmpty())
        return;

    //节流：距上次写盘未超过阈值则跳过
    const qint64 now = QDateTime::currentMSecsSinceEpoch();
    if (!force)
    {
        qint64 expected = m_lastPersistMs.load(std::memory_order_relaxed);
        if (now - expected < kPersistThrottleMs)
            return;
        //CAS 抢占本次写盘权限，避免多线程重复写
        if (!m_lastPersistMs.compare_exchange_strong(expected, now, std::memory_order_relaxed))
            return;
    }
    else
    {
        m_lastPersistMs.store(now, std::memory_order_relaxed);
    }

    QList<SyncTaskPtr> snapshot;
    {
        QMutexLocker locker(&m_stateMutex);
        snapshot = m_tasks.values();
    }
    TaskStateStore::save(snapshot, m_stateFile);
}

void TaskScheduler::checkAllFinished()
{
    bool done = false;
    {
        QMutexLocker locker(&m_stateMutex);
        done = (m_enqueued > 0 && m_finished >= m_enqueued);
    }
    if (done)
        emit allFinished();
}
