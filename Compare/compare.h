#ifndef COMPARE_H
#define COMPARE_H

#include <functional>
#include <atomic>
#include "scandata.h"
#include "ihashstrategy.h"

namespace Compare {

DiffItemList compare(const FileMetaHash& mapA,
                     const FileMetaHash& mapB,
                     const BaselineHash& baseline,
                     IHashStrategy*     hasher,
                     const QString&     rootA,
                     const QString&     rootB,
                     const ScanOptions& opt,
                     const std::function<void(int, int)>& onProgress,
                     const std::atomic<bool>* cancel);

}

#endif // COMPARE_H
