#ifndef DISPATCHENGINE_H
#define DISPATCHENGINE_H

#include <QObject>
#include <QString>
#include "scandata.h"
#include "scanworker.h"
#include "filemoinitor.h"
#include "taskscheduler.h"
#include "synctask.h"
#include <QThread>
#include <atomic>
#include "logdata.h"
#include <QTimer>

//调度引擎
class DispatchEngine : public QObject
{
    Q_OBJECT
public:
    explicit DispatchEngine(QObject *parent = nullptr);
    ~DispatchEngine();

    void setCompareDirs(const QString &dir_A,const QString &dir_B);
    inline QString getDirA() const {return  m_dir_A;}
    inline QString getDirB() const {return  m_dir_B;}

    void setOptions(const ScanOptions& option);
    inline ScanOptions getOptions() const {return m_option;}

    inline QString getWorkSpace() const {return  m_workspace;}
    void setWorkSpace(const QString& dir);

    //扫描
    void startScan();
    void cancelScan();
    inline bool isScanning() const {return  m_scanning;}
    inline DiffItemList getDifferentList() const {return  m_diffitemList;}
    //回写用户勾选/方向（由 UI 调用后执行 startSync）
    inline void setDifferentList(const DiffItemList& items) {m_diffitemList = items;}

    //同步
    void startSync();
    inline bool isSyncing() const {return  m_syncing;}
    void pauseAllSync();
    void resumeAllSync();
    void cancelAllSync();
    void pauseSyncTask(int id);
    void resumeSyncTask(int id);
    void cancelSyncTask(int id);
    inline QList<SyncTaskPtr> getSyncTasks() const {return  m_syncTasks;}

    //崩溃恢复
    QList<SyncTaskPtr> loadUnfinishedTasks() const;
    void resumeTasks(const QList<SyncTaskPtr>& tasks);
    void clearUnfinishedTasks();

signals:
    //调度消息
    void dispatchSystemData(const QString &msg,QDateTime time,SYSTEMLOGTYPE type);
    //扫描进度
    void scanProgress(int percent, const QString& text);
    //扫描完成
    void scanFinished(const DiffItemList& items);
    //扫描失败
    void scanFailed(const QString& error);
    //差异变化
    void differentChanged();

    //监控目录
    void MoinitorNotified(const QStringList& dirs);

    //同步任务
    void syncTaskStarted(int id);
    void syncTaskProgress(int id, qint64 done, qint64 total);
    void syncTaskStateChanged(int id, int state, const QString& error);
    void syncTaskFinished(int id, int state, const QString& error);
    void syncOverallProgress(qint64 done, qint64 total);
    void syncAllFinished();

protected:
    void initObj();
    void loadBaseline();
    void saveBaseline();
    void resetBaseline();
    void rebuildMoinitor();

protected slots:
    void ScanFinished_Slots(const DiffItemList& items, const FileMetaHash& mapA, const FileMetaHash& mapB);
    void ScanFailed_Slots(const QString& error);
    void FileMoinitorDirChanged_slots(const QStringList& dirs);
    void SaveTaskStateTimeout_slots();
    void MoinitorRefreshTimeout_Slots();
    void SyncTaskFinished_Slots(int id, int state, const QString& error);
    void SyncAllFinished_Slots();

private:
    //扫描目录
    QString m_dir_A;
    QString m_dir_B;
    //工作目录
    QString m_workspace;
    //扫描选项
    ScanOptions m_option;
    //差异列表
    DiffItemList m_diffitemList;
    //目录A文件元
    FileMetaHash m_hashA;
    //目录B文件元
    FileMetaHash m_hashB;
    //基线
    BaselineHash m_baseline;

    //扫描引擎
    ScanWorker* m_scanWorker = nullptr;
    //扫描线程
    QThread* m_scanThread = nullptr;
    //扫描取消标志位
    std::atomic<bool> m_scanCancel;

    //文件监控
    FileMoinitor* m_moinitor = nullptr;
    //监控刷新计时器
    QTimer* m_moinitorRefreshTimer = nullptr;

    //同步调度器
    TaskScheduler* m_scheduler = nullptr;
    //同步任务列表（按入队顺序）
    QList<SyncTaskPtr> m_syncTasks;
    //同步状态
    bool m_syncing = false;

    //保存任务（崩溃重启）
    QTimer* m_saveTaskTimer = nullptr;

    //扫描状态
    bool m_scanning = false;
};

#endif // DISPATCHENGINE_H
