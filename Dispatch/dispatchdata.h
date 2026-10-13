#ifndef DISPATCHDATA_H
#define DISPATCHDATA_H

//扫描线程名
#define SCANTHREAD_OBJNAME "ScanThread"

//基线文件名
#define BASELINE_FILENAME "baseline.json"

#include <QString>

//工作目录后缀
const QString WorkSpanceLatin = "/.FileSyncTool";

//监控刷新时间间隔
const int MoinitorRefreshInterval = 800;

//保存任务状态间隔
const int SaveTaskStateInterval = 2000;

//基线文件字段
const QString BaseLineFile = "files";
const QString BaseLineSize = "size";
const QString BaseLineTime = "mtime";
const QString BaseLineHash = "hash";
const QString BaseLineVersion = "version";

#endif // DISPATCHDATA_H
