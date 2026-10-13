#ifndef SCANDATA_H
#define SCANDATA_H
//扫描
#include <QString>
#include "differentdata.h"
#include "syncdata.h"
#include <QVector>
#include <QHash>
#include <QMap>

//文件元数据
struct FileMeta
{
    QString relPath;            //路径
    qint64  size      = -1;     //大小
    qint64  mtime     = 0;      //最后修改时间
    QString hash;               //内容哈希
    bool    exists    = false;  //存在
    bool    hashValid = false;  //有效（扫描前不计算，扫描后计算可能为空）
    bool    isDir = false;      //是否是目录
};

//差异项
struct FileDiffItem
{
    QString       relPath;
    DiffType      type    = DiffType::DiffType_Same;
    FileMeta      meta_A;
    FileMeta      meta_B;
    SyncSuggDirection dir     = SyncSuggDirection::SyncSuggDirection_None;
    bool          checked = false;
    QString       note;         //(冲突原因)
};

//基线记录
struct BaselineEntry {
    qint64  size  = -1;
    qint64  mtime = 0;
    QString hash;
};

typedef QVector<FileDiffItem> DiffItemList;
typedef QHash<QString, FileMeta> FileMetaHash;
typedef QHash<QString, BaselineEntry> BaselineHash; //<文件，基线记录>


//扫描选项
struct ScanOptions
{
    bool forceHash = false;     //深度校验
    bool listSameFiles = false;     //列出相同文件
    bool mirrorDelete = false;      //镜像删除
    QString  hashlogo           = QStringLiteral("MD5");    //哈希算法标识
    qint64   mtimeToleranceMs   = 2000;     //时间修改容差
    SyncMode mode               = SyncMode::SyncMode_TwoWay;
};

#endif // SCANDATA_H
