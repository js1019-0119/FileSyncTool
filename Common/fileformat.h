#ifndef FILEFORMAT_H
#define FILEFORMAT_H

#include <QString>
#include <QDateTime>

//文件大小格式化
inline QString formatFileSize(qint64 bytes)
{
    if (bytes < 0)
        return QStringLiteral("-");
    if (bytes < 1024)
        return QStringLiteral("%1 B").arg(bytes);

    const double kb = 1024.0;
    double v = bytes;
    const char* units[] = {"KB", "MB", "GB", "TB"};
    int i = 0;
    v /= kb;
    while (v >= kb && i < 3)
    {
        v /= kb;
        ++i;
    }
    return QStringLiteral("%1 %2").arg(v, 0, 'f', v >= 100 ? 0 : 1).arg(QString::fromLatin1(units[i]));
}

//修改时间格式化
inline QString formatMtime(qint64 mtimeMs)
{
    if (mtimeMs <= 0)
        return QStringLiteral("-");
    return QDateTime::fromMSecsSinceEpoch(mtimeMs).toString(QStringLiteral("yyyy-MM-dd HH:mm:ss"));
}

#endif // FILEFORMAT_H
