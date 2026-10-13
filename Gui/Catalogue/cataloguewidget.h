#ifndef CATALOGUEWIDGET_H
#define CATALOGUEWIDGET_H

#include <QWidget>
#include <QString>
#include <QDateTime>
#include "logdata.h"

namespace Ui {
class CatalogueWidget;
}

class CatalogueWidget : public QWidget
{
    Q_OBJECT

public:
    explicit CatalogueWidget(QWidget *parent = nullptr);
    ~CatalogueWidget();

    inline QString getDirA() const {return  m_dir_A;}
    inline QString getDirB() const {return  m_dir_B;}

    //程序内触发的目录浏览（供菜单等调用）
    void browseDirA();
    void browseDirB();
    //程序设置目录（供恢复持久化目录用）
    void setDirectoryA(const QString& dir);
    void setDirectoryB(const QString& dir);


signals:
    void setDirA(const QString &dir);
    void setDirB(const QString &dir);

public slots:
    void btn_SkimthroughA_Clicked();
    void btn_SkimthrougnB_Clicked();

protected:
    void initGui();
    void initConnect();
    //打开目录（startDir 为起始位置）
    QString openCatalogue(const QString& startDir);
    //拖拽目录
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dropEvent(QDropEvent* event) override;

private:
    Ui::CatalogueWidget *ui;
    QString m_dir_A;
    QString m_dir_B;
};

#endif // CATALOGUEWIDGET_H
