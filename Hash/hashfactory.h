#ifndef HASHFACTORY_H
#define HASHFACTORY_H

#include "ihashstrategy.h"
#include <QCryptographicHash>
#include <QStringList>

class HashStrategy : public IHashStrategy
{
public:
    explicit HashStrategy(QCryptographicHash::Algorithm algo);
    QString name() const override;
    QString hashFile(const QString& path, const std::atomic<bool>* cancel) const override;

private:
    QCryptographicHash::Algorithm m_algo;
    QString m_name;
};

class HashFactory
{
public:
    //支持算法
    static QStringList     available();
    static IHashStrategy*  create(const QString& name);
    //默认算法
    static QString         defaultAlgorithm();
};

#endif // HASHFACTORY_H
