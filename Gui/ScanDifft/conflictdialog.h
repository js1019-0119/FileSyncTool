#ifndef CONFLICTDIALOG_H
#define CONFLICTDIALOG_H

#include <QDialog>
#include "scandata.h"

//冲突裁决对话框：展示两边的大小、修改时间、哈希，供用户判断并选择保留哪一边
class ConflictDialog : public QDialog
{
    Q_OBJECT
public:
    explicit ConflictDialog(const FileDiffItem& item,
                            const QString& dirA, const QString& dirB,
                            QWidget* parent = nullptr);

    SyncSuggDirection chosen() const { return m_chosen; }

private:
    SyncSuggDirection m_chosen = SyncSuggDirection::SyncSuggDirection_None;
};

#endif // CONFLICTDIALOG_H
