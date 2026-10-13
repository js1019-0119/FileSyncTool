#ifndef TASKWIDGET_H
#define TASKWIDGET_H

#include <QWidget>
#include <QList>
#include <QPoint>
#include "synctask.h"

namespace Ui {
class TaskWidget;
}

class TaskTableModel;
class TaskProgressDelegate;
class QProgressBar;
class QLabel;

class TaskWidget : public QWidget
{
    Q_OBJECT

public:
    explicit TaskWidget(QWidget *parent = nullptr);
    ~TaskWidget();

    void setTasks(const QList<SyncTaskPtr>& tasks);
    void updateTask(int id);
    void setOverallProgress(qint64 done, qint64 total);
    void setOverallText(const QString& text);

signals:
    void StartAllClicked();
    void PauseAllClicked();
    void CancelAllClicked();
    //单个任务操作（右键菜单）
    void PauseTask(int id);
    void ResumeTask(int id);
    void CancelTask(int id);

protected:
    void initGui();
    void initConnect();

protected slots:
    void table_ContextMenu_slots(const QPoint& pos);

private:
    Ui::TaskWidget *ui;
    TaskTableModel* m_model = nullptr;
    TaskProgressDelegate* m_delegate = nullptr;
    QProgressBar* m_overallBar = nullptr;
    QLabel* m_overallLabel = nullptr;
};

#endif // TASKWIDGET_H
