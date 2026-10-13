#ifndef TASKSCHEDULER_H
#define TASKSCHEDULER_H

#include <QObject>
#include <QList>
#include <QHash>
#include <QMutex>
#include <QWaitCondition>
#include <queue>
#include <vector>
#include <atomic>
#include "synctask.h"

class TransferWorker;

//任务调度器
//自建线程池
//小文件优先队列
class TaskScheduler : public QObject
{
    Q_OBJECT
public:
    explicit TaskScheduler(int maxThreads = SyncMaxThreads, QObject* parent = nullptr);
    ~TaskScheduler();

    void start();
    void stop();

    void enqueue(const SyncTaskPtr& task);
    void pauseTask(int id);
    void resumeTask(int id);
    void cancelTask(int id);
    void pauseAll();
    void resumeAll();
    void cancelAll();

    SyncTaskPtr taskById(int id) const;
    QList<SyncTaskPtr> allTasks() const;

    void setStateFilePath(const QString& path);

    //供工作线程调用的线程安全接口
    SyncTaskPtr takeNext();
    void onTaskStarted(TransferWorker* w, const SyncTaskPtr& t);
    void onTaskProgress(const SyncTaskPtr& t, qint64 done, qint64 total);
    void onTaskStateChanged(const SyncTaskPtr& t);
    void onTaskFinished(TransferWorker* w, const SyncTaskPtr& t, TaskState st, const QString& err);
    //持久化任务状态（force=true 强制写）
    void persistTasks(bool force = false);

signals:
    void taskStarted(int id);
    void taskProgress(int id, qint64 done, qint64 total);
    void taskStateChanged(int id, int state, const QString& error);
    void taskFinished(int id, int state, const QString& error);
    void overallProgress(qint64 done, qint64 total);
    void allFinished();

private:
    struct SizeGreater
    {
        bool operator()(const SyncTaskPtr& a, const SyncTaskPtr& b) const
        {
            return a->totalSize > b->totalSize;   //最小堆：小文件优先
        }
    };

    void checkAllFinished();

    //优先级队列 + 等待条件
    std::priority_queue<SyncTaskPtr, std::vector<SyncTaskPtr>, SizeGreater> m_queue;
    QMutex m_queueMutex;
    QWaitCondition m_queueCond;

    QList<TransferWorker*> m_workers;
    int m_maxThreads;
    std::atomic<bool> m_stopping{false};
    int m_nextId = 1;

    //调度器状态（m_stateMutex 保护）
    mutable QMutex m_stateMutex;
    QHash<int, SyncTaskPtr> m_tasks;
    QHash<int, TransferWorker*> m_taskWorker;
    QHash<int, qint64> m_lastDone;
    qint64 m_totalBytes = 0;
    qint64 m_doneBytes = 0;
    int m_enqueued = 0;
    int m_finished = 0;

    QString m_stateFile;

    //持久化节流，合并高频写入
    std::atomic<qint64> m_lastPersistMs{0};
};

#endif // TASKSCHEDULER_H
