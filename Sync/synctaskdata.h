#ifndef SYNCTASKDATA_H
#define SYNCTASKDATA_H

#include <QString>

//分块大小（断点续传）
const qint64 SyncChunkSize = 4 * 1024 * 1024;   // 4MB

//最大并发线程数
const int SyncMaxThreads = 4;

//断点续传临时分片文件后缀
const QString SyncTmpPartSuffix = ".fspart";

//任务状态持久化文件名
const QString SyncTaskStateFilename = "taskstate.json";

#endif // SYNCTASKDATA_H
