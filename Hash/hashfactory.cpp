#include "hashfactory.h"
#include <QFile>
#include "hashdata.h"

HashStrategy::HashStrategy(QCryptographicHash::Algorithm algo)
    : m_algo(algo)
{
    switch (algo)
    {
    case QCryptographicHash::Md5:
        m_name = QStringLiteral(HASH_LOGO_MD5);
        break;
    case QCryptographicHash::Sha1:
        m_name = QStringLiteral(HASH_LOGO_SHA1);
        break;
    case QCryptographicHash::Sha256:
        m_name = QStringLiteral(HASH_LOGO_SHA256);
        break;
    default:
        m_name = QStringLiteral(HASH_LOGO_MD5);
        break;
    }
}

QString HashStrategy::name() const
{
    return m_name;
}

QString HashStrategy::hashFile(const QString& path, const std::atomic<bool>* cancel) const
{
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly))
        return QString();

    QCryptographicHash h(m_algo);

    const int kBuf = 256 * 1024;

    //不初始化
    QByteArray buf(kBuf, Qt::Uninitialized);

    //每次只占256KB,避免大文件一次性读入
    while (!f.atEnd())
    {
        if (cancel && cancel->load())
            return QString();
        const qint64 n = f.read(buf.data(), kBuf);
        if (n < 0)
            return QString();
        if (n == 0)
            break;
        h.addData(buf.constData(), int(n));
    }
    f.close();
    return QString::fromLatin1(h.result().toHex());
}

QStringList HashFactory::available()
{
    return QStringList() << QStringLiteral(HASH_LOGO_MD5)
                         << QStringLiteral(HASH_LOGO_SHA1)
                         << QStringLiteral(HASH_LOGO_SHA256);
}

QString HashFactory::defaultAlgorithm()
{
    return QStringLiteral(HASH_LOGO_MD5);
}

//默认
IHashStrategy* HashFactory::create(const QString& name)
{
    const QString n = name.trimmed().toUpper();
    if (n == QLatin1String(HASH_LOGO_SHA1))
        return new HashStrategy(QCryptographicHash::Sha1);
    if (n == QLatin1String(HASH_LOGO_SHA256))
        return new HashStrategy(QCryptographicHash::Sha256);
    return new HashStrategy(QCryptographicHash::Md5);
}
