#ifndef SCANWORKER_H
#define SCANWORKER_H

#include <QObject>
#include <atomic>
#include "scandata.h"
#include "logdata.h"
#include <QDateTime>

class ScanWorker : public QObject
{
    Q_OBJECT
public:
    explicit ScanWorker(QObject *parent = nullptr);

signals:
    //进度
    void progress(int percent, const QString& text);
    void finished(const DiffItemList& items, const FileMetaHash& mapA, const FileMetaHash& mapB);
    void failed(const QString& error);
    void sendSystemLogMsg(const QString &msg,QDateTime time,SYSTEMLOGTYPE type);

public slots:
    void scanAll(const QString& rootA,const QString& rootB,
                 const ScanOptions& opt,const BaselineHash& baseline);

    void cancel();

private:
    //扫描收集文件
    bool scanDirectory(const QString& root, FileMetaHash& out,
                       const QString& label, QString& err);
    std::atomic<bool> m_cancel;
};

#endif // SCANWORKER_H
