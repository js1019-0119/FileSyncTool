#ifndef IHASHSTRATEGY_H
#define IHASHSTRATEGY_H

#include <QString>
#include <atomic>
//哈希策略
//接口抽象，可替换哈希算法
class IHashStrategy
{
public:
    virtual ~IHashStrategy() {}
    virtual QString name() const = 0;
    virtual QString hashFile(const QString& path,
                             const std::atomic<bool>* cancel = nullptr) const = 0;
};

#endif // IHASHSTRATEGY_H
