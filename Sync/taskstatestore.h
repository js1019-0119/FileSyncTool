#ifndef TASKSTATESTORE_H
#define TASKSTATESTORE_H

#include <QList>
#include <QString>
#include "synctask.h"

//任务状态持久化（断点续传 / 崩溃恢复）
class TaskStateStore
{
public:
    //默认状态文件路径
    static QString defaultPath(const QString& workspace);
    //保存未完成任务
    static void save(const QList<SyncTaskPtr>& tasks, const QString& path);
    //载入上次未完成任务
    static QList<SyncTaskPtr> load(const QString& path);
    //清除状态文件
    static void clear(const QString& path);
};

#endif // TASKSTATESTORE_H
