#include "mainwindow.h"
#include <QApplication>
#include <QFile>
#include <QTranslator>
#include "logdata.h"
#include "scandata.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    QApplication::setApplicationName(QStringLiteral("FileSyncTool"));
    QApplication::setOrganizationName(QStringLiteral("FileSyncTool"));
    QApplication::setApplicationVersion(QStringLiteral("1.0.0"));

    //加载中文翻译
    QTranslator translator;
    if (translator.load(QStringLiteral(":/res/qt_zh_CN.qm")))
        a.installTranslator(&translator);

    //加载全局样式
    QFile qss(QStringLiteral(":/res/app.qss"));
    if (qss.open(QIODevice::ReadOnly))
        a.setStyleSheet(QString::fromUtf8(qss.readAll()));

    qRegisterMetaType<SYSTEMLOGTYPE>("SYSTEMLOGTYPE");
    qRegisterMetaType<FileMeta>("FileMeta");
    qRegisterMetaType<FileMetaHash>("FileMetaHash");
    qRegisterMetaType<FileDiffItem>("FileDiffItem");
    qRegisterMetaType<DiffItemList>("DiffItemList");
    qRegisterMetaType<ScanOptions>("ScanOptions");
    //SyncTaskPtr 以 QSharedPointer 传递，无需跨线程注册
    //qRegisterMetaType<SyncTaskPtr>("SyncTaskPtr");

    MainWindow w;
    w.show();

    return a.exec();
}
