#ifndef TASKWORKER_H
#define TASKWORKER_H

#include <QThread>
#include <QMutex>
#include <QWaitCondition>
#include <atomic>
#include "synctask.h"

class TaskScheduler;

//传输工作线程：分块 + 断点续传复制
class TransferWorker : public QThread
{
    Q_OBJECT
public:
    TransferWorker(TaskScheduler* scheduler, int index, QObject* parent = nullptr);

    int index() const { return m_index; }
    void requestStop();
    void signalWake();

protected:
    void run() override;

private:
    enum class Ctrl { Ctrl_Go, Ctrl_Cancel, Ctrl_Stop };

    //检查取消/暂停/停机，返回后续处理方式
    Ctrl checkCtrl(const SyncTaskPtr& task);
    void doTask(const SyncTaskPtr& task);

    TaskScheduler* m_scheduler;
    int m_index;
    QMutex m_mutex;
    QWaitCondition m_cond;
    std::atomic<bool> m_stop{false};
};

#endif // TASKWORKER_H
