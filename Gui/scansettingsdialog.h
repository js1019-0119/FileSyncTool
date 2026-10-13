#ifndef SCANSETTINGSDIALOG_H
#define SCANSETTINGSDIALOG_H

#include <QDialog>
#include "scandata.h"

class QComboBox;
class QCheckBox;

//扫描设置对话框：仅设置扫描相关项（哈希算法、深度校验、列出相同、镜像删除）
class ScanSettingsDialog : public QDialog
{
    Q_OBJECT
public:
    explicit ScanSettingsDialog(const ScanOptions& current, QWidget* parent = nullptr);

    ScanOptions options() const;

private:
    void initGui(const ScanOptions& current);

    QComboBox* m_hashCombo = nullptr;
    QCheckBox* m_deepCheck = nullptr;
    QCheckBox* m_commonCheck = nullptr;
    QCheckBox* m_mirrorCheck = nullptr;
    SyncMode m_currentMode = SyncMode::SyncMode_TwoWay;
};

#endif // SCANSETTINGSDIALOG_H
