#ifndef SYNCTASK_H
#define SYNCTASK_H

#include <QString>
#include <QSharedPointer>
#include <atomic>
#include "taskdata.h"
#include "syncdata.h"
#include "synctaskdata.h"

//同步任务
class SyncTask
{
public:
    int      id         = 0;
    QString  sourcePath;   //源绝对路径
    QString  targetPath;   //目标绝对路径
    QString  relPath;      //相对路径
    qint64   totalSize = 0;
    qint64   doneSize  = 0;
    TaskState state    = TaskState::TaskState_Pending;
    SyncSuggDirection dir = SyncSuggDirection::SyncSuggDirection_None;
    QString  error;
    bool     isDir = false;

    //线程安全标志
    //取消
    std::atomic<bool> cancelFlag{false};
    //暂停
    std::atomic<bool> pauseFlag{false};

    //断点续传临时分片路径
    QString tempPath() const { return targetPath + SyncTmpPartSuffix; }

    static QString stateText(TaskState s);
    static bool isFinished(TaskState s);
};

//共享指针避免调度某处析构
typedef QSharedPointer<SyncTask> SyncTaskPtr;

#endif // SYNCTASK_H
