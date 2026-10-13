#include "tasktablemodel.h"
#include "fileformat.h"
#include <QColor>
#include <QPainter>

namespace
{
    //任务状态对应的主题色
    QColor stateColor(TaskState s)
    {
        switch (s)
        {
        case TaskState::TaskState_Completed:
            return QColor(0x188038);
        case TaskState::TaskState_Failed:
            return QColor(0xd93025);
        case TaskState::TaskState_Canceled:
            return QColor(0x9e9e9e);
        case TaskState::TaskState_Paused:
            return QColor(0xe37400);
        case TaskState::TaskState_Running:
            return QColor(0x1a73e8);
        default:
            return QColor(0x1a73e8);
        }
    }
}

TaskTableModel::TaskTableModel(QObject* parent)
    : QAbstractTableModel(parent)
{
}

void TaskTableModel::clear()
{
    beginResetModel();
    m_tasks.clear();
    m_rowOf.clear();
    endResetModel();
}

void TaskTableModel::setTasks(const QList<SyncTaskPtr>& tasks)
{
    beginResetModel();
    m_tasks.clear();
    m_rowOf.clear();
    //预留容量，避免大量任务时 QList/QHash 反复扩容
    m_tasks.reserve(tasks.size());
    m_rowOf.reserve(tasks.size());
    for (const SyncTaskPtr& t : tasks)
    {
        m_rowOf.insert(t->id, m_tasks.size());
        m_tasks.append(t);
    }
    endResetModel();
}

void TaskTableModel::updateTask(int id)
{
    const int row = rowOf(id);
    if (row >= 0)
        //只刷新会变化的列（已传/进度/状态），减少重绘范围，提升性能
        emit dataChanged(index(row, ColDone), index(row, ColState));
}

SyncTaskPtr TaskTableModel::taskAt(int row) const
{
    if (row < 0 || row >= m_tasks.size())
        return SyncTaskPtr();
    return m_tasks.at(row);
}

int TaskTableModel::rowOf(int id) const
{
    return m_rowOf.value(id, -1);
}

int TaskTableModel::rowCount(const QModelIndex& parent) const
{
    if (parent.isValid())
        return 0;
    return m_tasks.size();
}

int TaskTableModel::columnCount(const QModelIndex& parent) const
{
    if (parent.isValid())
        return 0;
    return ColumnCount;
}

QVariant TaskTableModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_tasks.size())
        return QVariant();

    const SyncTaskPtr t = m_tasks.at(index.row());
    const int col = index.column();

    if (role == Qt::DisplayRole)
    {
        switch (col)
        {
        case ColId:
            return t->id;
        case ColPath:
            return t->relPath;
        case ColSize:
            return t->isDir ? QStringLiteral("[目录]") : formatFileSize(t->totalSize);
        case ColDone:
            return t->isDir ? QStringLiteral("-") : formatFileSize(t->doneSize);
        case ColProgress: {
            if (t->isDir || t->totalSize <= 0)
                return QStringLiteral("-");
            return QStringLiteral("%1%").arg(int(t->doneSize * 100 / t->totalSize));
        }
        case ColState:
            return SyncTask::stateText(t->state);
        default: break;
        }
    }

    if (role == ProgressRole && col == ColProgress && !t->isDir && t->totalSize > 0)
        return int(t->doneSize * 100 / t->totalSize);

    if (role == StateColorRole && col == ColProgress)
        return stateColor(t->state);

    if (role == Qt::TextAlignmentRole)
    {
        if (col == ColSize || col == ColDone || col == ColProgress)
            return int(Qt::AlignRight | Qt::AlignVCenter);
    }

    if (role == Qt::ForegroundRole && col == ColState)
        return stateColor(t->state);

    if (role == Qt::ToolTipRole)
    {
        QString tip = QStringLiteral("任务 %1：%2\n源：%3\n目标：%4\n状态：%5")
                .arg(t->id).arg(t->relPath, t->sourcePath, t->targetPath, SyncTask::stateText(t->state));
        if (!t->error.isEmpty())
            tip += QStringLiteral("\n错误：%1").arg(t->error);
        return tip;
    }

    return QVariant();
}

QVariant TaskTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
    {
        switch (section)
        {
        case ColId:
            return QStringLiteral("编号");
        case ColPath:
            return QStringLiteral("文件");
        case ColSize:
            return QStringLiteral("大小");
        case ColDone:
            return QStringLiteral("已传");
        case ColProgress:
            return QStringLiteral("进度");
        case ColState:
            return QStringLiteral("状态");
        }
    }
    return QAbstractTableModel::headerData(section, orientation, role);
}

// TaskProgressDelegate
TaskProgressDelegate::TaskProgressDelegate(QObject* parent)
    : QStyledItemDelegate(parent)
{
}

void TaskProgressDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option,
                                 const QModelIndex& index) const
{
    const QVariant pv = index.data(TaskTableModel::ProgressRole);
    if (!pv.isValid())
    {
        QStyledItemDelegate::paint(painter, option, index);
        return;
    }

    const int percent = qBound(0, pv.toInt(), 100);
    const QColor color = index.data(TaskTableModel::StateColorRole).value<QColor>();

    painter->save();
    painter->setRenderHint(QPainter::Antialiasing);

    const QRectF r = option.rect.adjusted(4, 5, -4, -5);

    //轨道
    painter->setPen(Qt::NoPen);
    painter->setBrush(QColor(0, 0, 0, 22));
    painter->drawRoundedRect(r, 6, 6);

    //填充
    if (percent > 0)
    {
        const QRectF fill(r.x(), r.y(), r.width() * percent / 100.0, r.height());
        painter->setBrush(color);
        painter->drawRoundedRect(fill, 6, 6);
    }

    //文字：进度较小时用深色，否则用白色
    painter->setPen(percent < 12 ? QColor(0x555555) : QColor(Qt::white));
    QFont f = option.font;
    f.setPixelSize(11);
    f.setBold(true);
    painter->setFont(f);
    painter->drawText(r, Qt::AlignCenter, QStringLiteral("%1%").arg(percent));

    painter->restore();
}
