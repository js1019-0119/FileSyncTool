#ifndef TASKTABLEMODEL_H
#define TASKTABLEMODEL_H

#include <QAbstractTableModel>
#include <QStyledItemDelegate>
#include <QList>
#include <QHash>
#include "synctask.h"

//任务队列 Model
class TaskTableModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    enum Column {
        ColId = 0,
        ColPath,
        ColSize,
        ColDone,
        ColProgress,
        ColState,
        ColumnCount
    };

    static const int ProgressRole = Qt::UserRole + 1;   //0~100 百分比
    static const int StateColorRole = Qt::UserRole + 2; //状态色（进度条填充色）

    explicit TaskTableModel(QObject* parent = nullptr);

    void setTasks(const QList<SyncTaskPtr>& tasks);
    void updateTask(int id);
    SyncTaskPtr taskAt(int row) const;
    int rowOf(int id) const;
    void clear();

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    int columnCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;

private:
    QList<SyncTaskPtr> m_tasks;
    QHash<int, int> m_rowOf;
};

//进度条绘制委托：将"进度"列渲染为进度条
class TaskProgressDelegate : public QStyledItemDelegate
{
    Q_OBJECT
public:
    explicit TaskProgressDelegate(QObject* parent = nullptr);

    void paint(QPainter* painter, const QStyleOptionViewItem& option,
               const QModelIndex& index) const override;
};

#endif // TASKTABLEMODEL_H
