#ifndef SYNCMODEDIALOG_H
#define SYNCMODEDIALOG_H

#include <QDialog>
#include "syncdata.h"

//同步模式对话框：仅选择同步模式
class SyncModeDialog : public QDialog
{
    Q_OBJECT
public:
    explicit SyncModeDialog(SyncMode current, QWidget* parent = nullptr);

    SyncMode mode() const { return m_mode; }

private:
    SyncMode m_mode = SyncMode::SyncMode_TwoWay;
};

#endif // SYNCMODEDIALOG_H
