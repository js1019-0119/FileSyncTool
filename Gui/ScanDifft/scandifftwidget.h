#ifndef SCANDIFFTWIDGET_H
#define SCANDIFFTWIDGET_H

#include <QWidget>
#include "scandata.h"

namespace Ui {
class ScanDifftWidget;
}

class DiffTableModel;
class DiffFilterProxy;
class BadgeDelegate;

class ScanDifftWidget : public QWidget
{
    Q_OBJECT

public:
    explicit ScanDifftWidget(QWidget *parent = nullptr);
    ~ScanDifftWidget();

    void setbtnScanText(const QString &text);
    void setScanProgressValue(const int progress);
    void setScanProgressText(const QString &text);
    //扫描阶段忙碌动画（无法估算进度时）
    void setScanBusy(const QString &text);
    //复位扫描进度（完成/取消后归零）
    void resetScanProgress();

    //设置差异列表
    void setDiffItems(const DiffItemList& items);
    //获取差异列表（含用户勾选/方向修改）
    DiffItemList getDiffItems() const;
    //设置对比目录（用于冲突对话框展示）
    void setCompareDirs(const QString& dirA, const QString& dirB);
    //应用扫描设置到界面控件（供设置对话框/菜单调用）
    void applyScanOptions(const ScanOptions& options);

signals:
    void ScanOptionsChanged(const ScanOptions &options);
    void StartScan();
    void StartSync();

protected:
    void initGui();
    void initConnect();

protected slots:
    void cb_HashTextChanged_Slots(const QString &text);
    void ck_DeepCheckStateChanged_Slots(int state);
    void ck_ListCommonCheckStateChanged_Slots(int state);
    void ck_MirrorDeleteCheckStateChanged_Slots(int state);
    void cb_SyncModeTextChanged_Slots(const QString &text);
    void btn_ScanClicked_Slots();
    void cb_FilterChanged_Slots(int index);
    void le_FindTextChanged_Slots(const QString &text);
    void btn_SyncClicked_Slots();
    void btn_ConflictHandleClicked_Slots();
    void table_DoubleClicked_Slots(const QModelIndex& index);
    void table_ContextMenu_slots(const QPoint& pos);

private:
    void updateStats();
    void handleConflictAtRow(int row);

    Ui::ScanDifftWidget *ui;
    ScanOptions m_scanoptions;
    DiffTableModel* m_diffModel = nullptr;
    DiffFilterProxy* m_diffProxy = nullptr;
    BadgeDelegate* m_badgeDelegate = nullptr;
    QString m_dirA;
    QString m_dirB;
};

#endif // SCANDIFFTWIDGET_H
