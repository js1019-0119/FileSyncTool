#include "compare.h"
#include <QDir>
#include <QSet>
#include <QStringList>
#include <QtMath>
#include "differentdata.h"
#include "syncdata.h"

namespace
{

bool metaMatchesBaseline(const FileMeta& m, const BaselineEntry& e, qint64 tol)
{
    if (e.size < 0)
        return false;
    if (m.size != e.size)
        return false;
    if (!e.hash.isEmpty() && !m.hash.isEmpty())
        return m.hash == e.hash;
    return qAbs(m.mtime - e.mtime) <= tol;
}

} // namespace

DiffItemList Compare::compare(const FileMetaHash& mapA,
                                    const FileMetaHash& mapB,
                                    const BaselineHash& baseline,
                                    IHashStrategy*     hasher,
                                    const QString&     rootA,
                                    const QString&     rootB,
                                    const ScanOptions& opt,
                                    const std::function<void(int, int)>& onProgress,
                                    const std::atomic<bool>* cancel)
{
    DiffItemList result;

    //合并key排序
    QSet<QString> keySet;
    keySet.reserve(mapA.size() + mapB.size());
    for (FileMetaHash::const_iterator it = mapA.constBegin(); it != mapA.constEnd(); ++it)
        keySet.insert(it.key());
    for (FileMetaHash::const_iterator it = mapB.constBegin(); it != mapB.constEnd(); ++it)
        keySet.insert(it.key());

    QStringList keys = keySet.toList();
    keys.sort();

    const int total = keys.size();
    int done = 0;

    //预分配结果
    result.reserve(total);

    for (int ki = 0; ki < keys.size(); ++ki)
    {
        if (cancel && cancel->load())
            break;

        const QString& key = keys.at(ki);

        FileDiffItem item;
        // 去掉末尾的 '/'（目录标识）显示
        const bool isDirKey = key.endsWith(QLatin1Char('/'));
        item.relPath = isDirKey ? key.left(key.size() - 1) : key;

        //单次 constFind，避免两次哈希查找
        const FileMetaHash::const_iterator itA = mapA.constFind(key);
        const FileMetaHash::const_iterator itB = mapB.constFind(key);
        const bool inA = (itA != mapA.constEnd());
        const bool inB = (itB != mapB.constEnd());
        //目录归属
        if (inA)
            item.meta_A = itA.value();
        if (inB)
            item.meta_B = itB.value();

        //仅A有
        if (inA && !inB)
        {
            item.type = DiffType::DiffType_OnlyA;
            switch (opt.mode)
            {
            case SyncMode::SyncMode_BtoA:
                item.dir = opt.mirrorDelete ? SyncSuggDirection::SyncSuggDirection_RemoveInA : SyncSuggDirection::SyncSuggDirection_None;
                break;
            default:
                item.dir = SyncSuggDirection::SyncSuggDirection_AtoB;
                break;
            }
        }
        //仅B有
        else if (!inA && inB)
        {
            item.type = DiffType::DiffType_OnlyB;
            switch (opt.mode)
            {
            case SyncMode::SyncMode_AtoB:
                item.dir = opt.mirrorDelete ? SyncSuggDirection::SyncSuggDirection_RemoveInB : SyncSuggDirection::SyncSuggDirection_None;
                break;
            default:
                item.dir = SyncSuggDirection::SyncSuggDirection_BtoA;
                break;
            }
        }
        //两边都有
        else
        {
            // 目录只要两边都存在，就算相同，文件单独便利
            if (isDirKey)
            {
                item.type = DiffType::DiffType_Same;
                item.dir  = SyncSuggDirection::SyncSuggDirection_None;
            }
            else
            {
                bool same = false;
                bool needHash = true;

                //大小不一致
                if (item.meta_A.size != item.meta_B.size)
                {
                    same = false;
                    needHash = false;
                }
                //未开启深度校验，时间差在容差范围内
                else if (!opt.forceHash &&
                           qAbs(item.meta_A.mtime - item.meta_B.mtime) <= opt.mtimeToleranceMs)
                {
                    same = true;
                    needHash = false;
                }

                //上述都符合判断哈希
                if (needHash)
                {
                    QString ha = item.meta_A.hash;
                    QString hb = item.meta_B.hash;

                    if (ha.isEmpty())
                    {
                        ha = hasher->hashFile(QDir(rootA).absoluteFilePath(key), cancel);
                        item.meta_A.hash = ha;
                        item.meta_A.hashValid = !ha.isEmpty();
                    }
                    if (hb.isEmpty())
                    {
                        hb = hasher->hashFile(QDir(rootB).absoluteFilePath(key), cancel);
                        item.meta_B.hash = hb;
                        item.meta_B.hashValid = !hb.isEmpty();
                    }
                    same = (!ha.isEmpty() && ha == hb);
                }

                if (same)
                {
                    item.type = DiffType::DiffType_Same;
                    item.dir  = SyncSuggDirection::SyncSuggDirection_None;
                }
                else
                {
                    //哈希不一致
                    item.type = DiffType::DiffType_ContentDiff;

                    if (opt.mode == SyncMode::SyncMode_TwoWay)
                    {
                        //获取修改前基线
                        BaselineHash::const_iterator bit = baseline.constFind(key);
                        const bool hasBase = (bit != baseline.constEnd());

                        bool aChanged = true;
                        bool bChanged = true;

                        if (hasBase)
                        {
                            aChanged = !metaMatchesBaseline(item.meta_A, bit.value(), opt.mtimeToleranceMs);
                            bChanged = !metaMatchesBaseline(item.meta_B, bit.value(), opt.mtimeToleranceMs);
                        }

                        //两边都修改了
                        if (aChanged && bChanged)
                        {
                            item.type = DiffType::DiffType_Conflict;
                            item.dir  = SyncSuggDirection::SyncSuggDirection_None;
                            item.note = QStringLiteral("两边内容不同且都与上次同步基线不一致");
                        }
                        //只有B修改
                        else if (aChanged && !bChanged)
                        {
                            item.dir = SyncSuggDirection::SyncSuggDirection_AtoB;
                        }
                        //只有A修改
                        else if (!aChanged && bChanged)
                        {
                            item.dir = SyncSuggDirection::SyncSuggDirection_BtoA;
                        }
                        else
                        {
                            item.type = DiffType::DiffType_Conflict;
                            item.dir  = SyncSuggDirection::SyncSuggDirection_None;
                            item.note = QStringLiteral("基线失效，无法自动判定方向");
                        }
                    }
                    else if (opt.mode == SyncMode::SyncMode_AtoB)
                    {
                        item.dir = SyncSuggDirection::SyncSuggDirection_AtoB;
                    } else
                    {
                        item.dir = SyncSuggDirection::SyncSuggDirection_BtoA;
                    }
                }
            }
        }

        //默认勾选：非相同、非冲突、有明确方向的条目
        item.checked = (item.type != DiffType::DiffType_Same &&
                        item.type != DiffType::DiffType_Conflict &&
                        item.dir   != SyncSuggDirection::SyncSuggDirection_None);

        //相同文件仅在勾选"列出相同文件"时展示
        if (item.type != DiffType::DiffType_Same || opt.listSameFiles)
            result.append(item);

        ++done;
        if (onProgress && (done % 100 == 0 || done == total))
            onProgress(done, total);
    }

    if (onProgress)
        onProgress(total, total);
    return result;
}

