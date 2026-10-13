#include "synctask.h"

QString SyncTask::stateText(TaskState s)
{
    switch (s)
    {
    case TaskState::TaskState_Pending:
        return QStringLiteral("排队中");
    case TaskState::TaskState_Running:
        return QStringLiteral("传输中");
    case TaskState::TaskState_Paused:
        return QStringLiteral("已暂停");
    case TaskState::TaskState_Completed:
        return QStringLiteral("完成");
    case TaskState::TaskState_Failed:
        return QStringLiteral("失败");
    case TaskState::TaskState_Canceled:
        return QStringLiteral("已取消");
    }
    return QString();
}

bool SyncTask::isFinished(TaskState s)
{
    return s == TaskState::TaskState_Completed ||
           s == TaskState::TaskState_Failed ||
           s == TaskState::TaskState_Canceled;
}
