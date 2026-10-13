#include "taskworker.h"
#include "taskscheduler.h"
#include "synctaskdata.h"
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QDateTime>
#include <QElapsedTimer>
#include <QMutexLocker>

#ifdef Q_OS_WIN
#include <windows.h>
#else
#include <utime.h>
#endif

namespace
{
    //保留源文件修改时间，避免时间戳漂移
    bool setFileMtime(const QString& path, qint64 mtimeMs)
    {
        if (mtimeMs <= 0)
            return false;
#ifdef Q_OS_WIN
        const HANDLE h = CreateFileW(reinterpret_cast<LPCWSTR>(path.utf16()),
                                     FILE_WRITE_ATTRIBUTES,
                                     FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                                     nullptr, OPEN_EXISTING,
                                     FILE_ATTRIBUTE_NORMAL, nullptr);
        if (h == INVALID_HANDLE_VALUE)
            return false;
        //1601-01-01 与 1970-01-01 之间的毫秒差
        const qint64 epochDiffMs = 11644473600000LL;
        ULARGE_INTEGER ul;
        ul.QuadPart = (mtimeMs + epochDiffMs) * 10000;   //转 100ns 间隔
        FILETIME ft;
        ft.dwLowDateTime  = ul.LowPart;
        ft.dwHighDateTime = ul.HighPart;
        const BOOL ok = SetFileTime(h, nullptr, nullptr, &ft);
        CloseHandle(h);
        return ok != 0;
#else
        struct utimbuf t;
        t.actime = t.modtime = mtimeMs / 1000;
        return ::utime(QFile::encodeName(path).constData(), &t) == 0;
#endif
    }
}

TransferWorker::TransferWorker(TaskScheduler* scheduler, int index, QObject* parent)
    : QThread(parent), m_scheduler(scheduler), m_index(index)
{
}

void TransferWorker::requestStop()
{
    m_stop.store(true);
    m_cond.wakeAll();
}

void TransferWorker::signalWake()
{
    m_cond.wakeAll();
}

void TransferWorker::run()
{
    while (!m_stop.load())
    {
        SyncTaskPtr task = m_scheduler->takeNext();
        if (task.isNull())
            break;   //停机
        doTask(task);
    }
}

TransferWorker::Ctrl TransferWorker::checkCtrl(const SyncTaskPtr& task)
{
    while (true)
    {
        if (task->cancelFlag.load())
            return Ctrl::Ctrl_Cancel;
        if (m_stop.load())
            return Ctrl::Ctrl_Stop;
        if (task->pauseFlag.load())
        {
            if (task->state != TaskState::TaskState_Paused)
            {
                task->state = TaskState::TaskState_Paused;
                m_scheduler->onTaskStateChanged(task);
                m_scheduler->persistTasks();
            }
            QMutexLocker locker(&m_mutex);
            while (task->pauseFlag.load() && !task->cancelFlag.load() && !m_stop.load())
                m_cond.wait(&m_mutex);
            if (task->state == TaskState::TaskState_Paused)
            {
                task->state = TaskState::TaskState_Running;
                m_scheduler->onTaskStateChanged(task);
            }
            continue;
        }
        return Ctrl::Ctrl_Go;
    }
}

void TransferWorker::doTask(const SyncTaskPtr& task)
{
    //目录任务：仅创建目录
    if (task->isDir)
    {
        QDir().mkpath(task->targetPath);
        m_scheduler->onTaskFinished(this, task, TaskState::TaskState_Completed, QString());
        return;
    }

    Ctrl c = checkCtrl(task);
    if (c == Ctrl::Ctrl_Cancel)
    {
        m_scheduler->onTaskFinished(this, task, TaskState::TaskState_Canceled, QString());
        return;
    }
    if (c == Ctrl::Ctrl_Stop)
    {
        task->state = TaskState::TaskState_Paused;
        m_scheduler->onTaskFinished(this, task, TaskState::TaskState_Paused, QString());
        return;
    }

    task->state = TaskState::TaskState_Running;
    m_scheduler->onTaskStarted(this, task);

    //打开源文件
    QFile src(task->sourcePath);
    if (!src.open(QIODevice::ReadOnly))
    {
        m_scheduler->onTaskFinished(this, task, TaskState::TaskState_Failed, src.errorString());
        return;
    }

    //断点续传：以临时分片已写入大小作为续传偏移
    qint64 offset = 0;
    const QString tempPath = task->tempPath();
    const QFileInfo tempInfo(tempPath);
    if (tempInfo.exists() && tempInfo.size() > 0 && tempInfo.size() < task->totalSize)
        offset = tempInfo.size();

    //确保目标父目录存在
    QDir().mkpath(QFileInfo(task->targetPath).absolutePath());

    //打开临时分片文件
    QFile dst(tempPath);
    QIODevice::OpenMode mode = QIODevice::WriteOnly;
    if (offset > 0)
        mode |= QIODevice::Append;
    else
        mode |= QIODevice::Truncate;
    if (!dst.open(mode))
    {
        src.close();
        m_scheduler->onTaskFinished(this, task, TaskState::TaskState_Failed, dst.errorString());
        return;
    }

    task->doneSize = offset;
    if (offset > 0)
        src.seek(offset);

    QByteArray buf(SyncChunkSize, Qt::Uninitialized);
    QElapsedTimer throttle;
    throttle.start();
    bool failed = false;
    QString err;

    while (task->doneSize < task->totalSize)
    {
        c = checkCtrl(task);
        if (c == Ctrl::Ctrl_Cancel)
        {
            dst.close();
            QFile::remove(tempPath);   //取消：清理临时分片
            m_scheduler->onTaskFinished(this, task, TaskState::TaskState_Canceled, QString());
            src.close();
            return;
        }
        if (c == Ctrl::Ctrl_Stop)
        {
            dst.flush();
            dst.close();
            task->state = TaskState::TaskState_Paused;   //停机：保留分片与状态，供续传
            m_scheduler->persistTasks();
            m_scheduler->onTaskFinished(this, task, TaskState::TaskState_Paused, QString());
            src.close();
            return;
        }

        //4MB
        const qint64 toRead = qMin<qint64>(SyncChunkSize, task->totalSize - task->doneSize);
        const qint64 r = src.read(buf.data(), toRead);
        if (r <= 0)
        {
            failed = true;
            err = src.errorString();
            break;
        }
        const qint64 w = dst.write(buf.constData(), r);
        if (w != r)
        {
            failed = true;
            err = dst.errorString();
            break;
        }
        task->doneSize += w;

        //节流上报，避免信号洪峰拖慢 UI
        if (throttle.elapsed() >= 50)
        {
            m_scheduler->onTaskProgress(task, task->doneSize, task->totalSize);
            throttle.restart();
        }
    }

    dst.flush();
    dst.close();
    src.close();

    if (failed)
    {
        //保留临时分片与状态，可续传/重试
        task->state = TaskState::TaskState_Failed;
        task->error = err;
        m_scheduler->persistTasks();
        m_scheduler->onTaskFinished(this, task, TaskState::TaskState_Failed, err);
        return;
    }

    //重命名
    const qint64 srcMtime = QFileInfo(task->sourcePath).lastModified().toMSecsSinceEpoch();

    bool renamed = QFile::rename(tempPath, task->targetPath);
    if (!renamed && QFile::exists(task->targetPath))
    {
        //目标已存在时先移除（多数平台 rename 已支持覆盖，此处兜底）
        QFile::remove(task->targetPath);
        renamed = QFile::rename(tempPath, task->targetPath);
    }
    if (!renamed)
    {
        task->state = TaskState::TaskState_Failed;
        task->error = QStringLiteral("重命名失败：%1").arg(task->targetPath);
        m_scheduler->persistTasks();
        m_scheduler->onTaskFinished(this, task, TaskState::TaskState_Failed, task->error);
        return;
    }

    //同步修改时间
    setFileMtime(task->targetPath, srcMtime);

    task->doneSize = task->totalSize;
    m_scheduler->onTaskProgress(task, task->doneSize, task->totalSize);
    task->error.clear();
    m_scheduler->onTaskFinished(this, task, TaskState::TaskState_Completed, QString());
}
