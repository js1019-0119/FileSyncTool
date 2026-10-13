#include "taskstatestore.h"
#include "synctaskdata.h"
#include <QFile>
#include <QDir>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>

QString TaskStateStore::defaultPath(const QString& workspace)
{
    return QDir(workspace).absoluteFilePath(SyncTaskStateFilename);
}

void TaskStateStore::save(const QList<SyncTaskPtr>& tasks, const QString& path)
{
    QJsonArray arr;
    for (const SyncTaskPtr& t : tasks)
    {
        if (SyncTask::isFinished(t->state))
            continue;   //只持久化未完成的

        QJsonObject o;
        o.insert(QStringLiteral("source"), t->sourcePath);
        o.insert(QStringLiteral("target"), t->targetPath);
        o.insert(QStringLiteral("rel"), t->relPath);
        o.insert(QStringLiteral("size"), double(t->totalSize));
        o.insert(QStringLiteral("done"), double(t->doneSize));
        o.insert(QStringLiteral("state"), int(t->state));
        o.insert(QStringLiteral("dir"), int(t->dir));
        o.insert(QStringLiteral("isDir"), t->isDir);
        arr.append(o);
    }

    QFile f(path);
    if (f.open(QIODevice::WriteOnly | QIODevice::Truncate))
        f.write(QJsonDocument(arr).toJson(QJsonDocument::Compact));
}

QList<SyncTaskPtr> TaskStateStore::load(const QString& path)
{
    QList<SyncTaskPtr> result;
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly))
        return result;

    QJsonParseError err;
    const QJsonDocument doc = QJsonDocument::fromJson(f.readAll(), &err);
    if (err.error != QJsonParseError::NoError || !doc.isArray())
        return result;

    for (const QJsonValue& v : doc.array())
    {
        const QJsonObject o = v.toObject();
        SyncTaskPtr t(new SyncTask);
        t->sourcePath = o.value(QStringLiteral("source")).toString();
        t->targetPath = o.value(QStringLiteral("target")).toString();
        t->relPath    = o.value(QStringLiteral("rel")).toString();
        t->totalSize  = qint64(o.value(QStringLiteral("size")).toDouble());
        t->doneSize   = qint64(o.value(QStringLiteral("done")).toDouble());
        t->state      = static_cast<TaskState>(o.value(QStringLiteral("state")).toInt());
        t->dir        = static_cast<SyncSuggDirection>(o.value(QStringLiteral("dir")).toInt());
        t->isDir      = o.value(QStringLiteral("isDir")).toBool();
        if (!t->sourcePath.isEmpty())
            result.append(t);
    }
    return result;
}

void TaskStateStore::clear(const QString& path)
{
    QFile::remove(path);
}
