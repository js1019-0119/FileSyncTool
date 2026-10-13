#ifndef FILEMOINITOR_H
#define FILEMOINITOR_H

#include <QObject>
#include <QFileSystemWatcher>
#include <QTimer>
#include <QThread>
#include <QMutex>
#include <QStringList>
#include <QSet>
#include <atomic>
#include "logdata.h"
#include <QDateTime>

//后台目录收集线程
//递归收集子目录，放到后台线程避免阻塞
class DirCollector : public QThread
{
    Q_OBJECT
public:
    explicit DirCollector(QObject *parent = nullptr);

    //发起一次收集：设置根目录与上限后启动线程
    void startCollect(const QStringList &roots, int max);
    //请求中断当前遍历
    void requestStop();

signals:
    //收集完成（后台线程发出，跨线程自动排队）
    void collected(const QStringList &dirs);

protected:
    void run() override;

private:
    QStringList m_roots;          //待遍历根目录
    int m_max = 4000;             //目录数量上限
    QMutex m_mutex;               //保护 m_roots
    std::atomic<bool> m_stop{false};
};


class FileMoinitor : public QObject
{
    Q_OBJECT
public:
    explicit FileMoinitor(QObject *parent = nullptr);
    ~FileMoinitor();

    void setRoots(const QStringList& roots);
    void start();
    void stop();
    inline bool isRunning() const {return  m_running;}
    int getMonitorDirCount() const {return  m_watch_set.size();}

signals:
    void FileMonitorStateMsg(const QString &msg);
    void sendSystemLogMsg(const QString &msg,QDateTime time,SYSTEMLOGTYPE type);
    void directoryChanged(const QStringList& dirs);

private slots:
    void MoinitorDirChanged_slots(const QString &p_dir);
    void TimeOutFlush_slots();
    //后台收集完成回调（主线程执行，批量 addPath）
    void onCollected(const QStringList &dirs);

protected:
    void initObj();

private:
    //发起后台目录收集（start 与增量刷新共用）
    void scheduleCollect();
    //增量加入监控（跳过已存在的目录）
    void addPathsIncremental(const QStringList &dirs);

private:
    QFileSystemWatcher* m_watcher = nullptr;
    QTimer* m_time = nullptr;
    //后台目录收集线程
    DirCollector* m_collector = nullptr;

    const int m_time_interval = 400;
    QStringList         m_roots;
    bool                m_running  = false;
    QSet<QString> m_watch_set;      //监控目录列表
    QSet<QString> m_pending_set;    //待刷新目录
    const int m_max_monitorSize = 4000;
    //是否已打印过监控启动日志（避免增量刷新刷屏）
    bool m_loggedOnce = false;
};

#endif // FILEMOINITOR_H
