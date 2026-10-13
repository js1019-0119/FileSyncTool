#include "scanworker.h"
#include <QFileInfo>
#include <QDir>
#include <QDirIterator>
#include <QDateTime>
#include <QScopedPointer>
#include "hashfactory.h"
#include "compare.h"

ScanWorker::ScanWorker(QObject *parent) : QObject(parent)
{
    m_cancel.store(false);
}

void ScanWorker::scanAll(const QString &rootA, const QString &rootB, const ScanOptions &opt, const BaselineHash &baseline)
{
    m_cancel.store(false);

    FileMetaHash mapA, mapB;
    QString err;

    emit progress(0, QStringLiteral("扫描目录 A…"));
    if (!scanDirectory(rootA, mapA, QStringLiteral("目录 A"), err))
    {
        //扫描A失败
        if (m_cancel.load())
            emit failed(QStringLiteral("扫描已取消"));
        else
            emit failed(err);
        return;
    }
    if (m_cancel.load())
    {
        emit failed(QStringLiteral("扫描已取消"));
        return;
    }

    emit progress(25, QStringLiteral("扫描目录 B…"));
    if (!scanDirectory(rootB, mapB, QStringLiteral("目录 B"), err))
    {
        //扫描B失败
        if (m_cancel.load())
            emit failed(QStringLiteral("扫描已取消"));
        else
            emit failed(err);
        return;
    }
    if (m_cancel.load())
    {
        emit failed(QStringLiteral("扫描已取消"));
        return;
    }

    emit progress(50, QStringLiteral("对比差异（大小 / 时间 / 哈希）…"));

    //哈希
    QScopedPointer<IHashStrategy> hasher(HashFactory::create(opt.hashlogo));

    DiffItemList items = Compare::compare(
        mapA, mapB, baseline, hasher.data(), rootA, rootB, opt,
        [this](int done, int total)
        {
            if (total <= 0) return;
            const int pct = 50 + (done * 50) / total;
            emit progress(pct, QStringLiteral("对比中 %1/%2 …").arg(done).arg(total));
        },
        &m_cancel);

    if (m_cancel.load())
    {
        emit failed(QStringLiteral("扫描已取消"));
        return;
    }

    emit progress(100, QStringLiteral("扫描完成，差异项 %1 个").arg(items.size()));
    emit sendSystemLogMsg(QStringLiteral("扫描完成：A=%1 文件, B=%2 文件, 差异=%3")
                 .arg(mapA.size()).arg(mapB.size()).arg(items.size()),
                          QDateTime::currentDateTime(),SYSTEMLOGTYPE::SYSTEMLOGTYPE_INFORMATION);
    emit finished(items, mapA, mapB);
}

void ScanWorker::cancel()
{
    m_cancel.store(true);
}

bool ScanWorker::scanDirectory(const QString &root, FileMetaHash &out, const QString &label, QString &err)
{
    QFileInfo rootInfo(root);
    if (!rootInfo.exists() || !rootInfo.isDir())
    {
        err = QStringLiteral("目录不存在或不可访问：%1").arg(root);
        return false;
    }

    //盘符根目录已带尾斜杠，避免重复追加
    const QString cleanRoot = QDir::cleanPath(rootInfo.absoluteFilePath());
    QString prefix = cleanRoot;
    if (!prefix.endsWith(QLatin1Char('/')))
        prefix += QLatin1Char('/');
    const int prefixLen = prefix.length();

    int count = 0;

    //单次遍历目录+文件，跳过符号链接避免死循环
    //跳过符号链接
    QDirIterator it(cleanRoot,
                    QDir::Dirs | QDir::Files | QDir::NoDotAndDotDot | QDir::Hidden | QDir::NoSymLinks,
                    QDirIterator::Subdirectories);

    while (it.hasNext())
    {
        if (m_cancel.load())
            return false;
        it.next();

        const QFileInfo fi = it.fileInfo();

        //filePath() 已是规范绝对路径，无需再 cleanPath
        const QString abs = it.filePath();
        if (abs.length() < prefixLen || !abs.startsWith(prefix))
            continue;

        FileMeta m;
        m.relPath = abs.mid(prefixLen);
        //最后修改时间到当前的时间戳
        m.mtime   = fi.lastModified().toMSecsSinceEpoch();
        m.exists  = true;

        if (fi.isDir())
        {
            m.size  = 0;
            m.isDir = true;
            //用 "路径末尾加 /" 作为 key，避免和同名文件冲突
            out.insert(m.relPath + QLatin1Char('/'), m);
        }
        else
        {
            //过滤同步临时分片文件
            const QString name = fi.fileName();
            if (name.endsWith(QLatin1String(".fstmp")) ||
                name.endsWith(QLatin1String(".fspart")))
                continue;

            m.size  = fi.size();
            m.isDir = false;
            out.insert(m.relPath, m);
        }

        if ((++count % 200) == 0)
        {
            //扫描阶段无法估算总量，进度条用忙碌动画 + 实时条目数
            emit progress(-1, QStringLiteral("%1：已扫描 %2 个条目…").arg(label).arg(count));
        }
    }

    emit progress(-1, QStringLiteral("%1：共 %2 个条目（含目录）").arg(label).arg(out.size()));
    return true;
}
