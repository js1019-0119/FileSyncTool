#include "difftablemodel.h"
#include "fileformat.h"
#include "badgedelegate.h"
#include <QColor>

namespace
{
    //差异类型对应的主题色
    QColor typeColor(DiffType t)
    {
        switch (t)
        {
        case DiffType::DiffType_Same:
            return QColor(Qt::gray);
        case DiffType::DiffType_OnlyA:
            return QColor(0x1a73e8);
        case DiffType::DiffType_OnlyB:
            return QColor(0x188038);
        case DiffType::DiffType_ContentDiff:
            return QColor(0xe37400);
        case DiffType::DiffType_Conflict:
            return QColor(0xd93025);
        }
        return QColor(Qt::gray);
    }
}

DiffTableModel::DiffTableModel(QObject* parent)
    : QAbstractTableModel(parent)
{
}

void DiffTableModel::setItems(const DiffItemList& items)
{
    beginResetModel();
    m_items = items;
    endResetModel();
}

FileDiffItem DiffTableModel::itemAt(int row) const
{
    if (row < 0 || row >= m_items.size())
        return FileDiffItem();
    return m_items.at(row);
}

void DiffTableModel::setChecked(int row, bool checked)
{
    if (row < 0 || row >= m_items.size())
        return;
    m_items[row].checked = checked;
    emit dataChanged(index(row, ColChecked), index(row, ColChecked));
}

void DiffTableModel::setDirection(int row, SyncSuggDirection dir)
{
    if (row < 0 || row >= m_items.size())
        return;
    m_items[row].dir = dir;
    m_items[row].checked = (dir != SyncSuggDirection::SyncSuggDirection_None);
    emit dataChanged(index(row, ColDirection), index(row, ColDirection));
    emit dataChanged(index(row, ColChecked), index(row, ColChecked));
}

void DiffTableModel::setAllChecked(bool checked)
{
    if (m_items.isEmpty())
        return;
    for (FileDiffItem& d : m_items)
    {
        if (checked)
            d.checked = (d.dir != SyncSuggDirection::SyncSuggDirection_None);
        else
            d.checked = false;
    }
    emit dataChanged(index(0, ColChecked), index(m_items.size() - 1, ColChecked));
}

void DiffTableModel::invertChecked()
{
    if (m_items.isEmpty())
        return;
    for (FileDiffItem& d : m_items)
        d.checked = !d.checked;
    emit dataChanged(index(0, ColChecked), index(m_items.size() - 1, ColChecked));
}

int DiffTableModel::countByType(DiffType t) const
{
    int n = 0;
    for (const FileDiffItem& d : m_items)
        if (d.type == t)
            ++n;
    return n;
}

int DiffTableModel::selectedCount() const
{
    int n = 0;
    for (const FileDiffItem& d : m_items)
        if (d.checked && d.dir != SyncSuggDirection::SyncSuggDirection_None)
            ++n;
    return n;
}

int DiffTableModel::rowCount(const QModelIndex& parent) const
{
    if (parent.isValid())
        return 0;
    return m_items.size();
}

int DiffTableModel::columnCount(const QModelIndex& parent) const
{
    if (parent.isValid())
        return 0;
    return ColumnCount;
}

QVariant DiffTableModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_items.size())
        return QVariant();

    const FileDiffItem& d = m_items.at(index.row());
    const int col = index.column();

    if (role == Qt::CheckStateRole && col == ColChecked)
        return d.checked ? Qt::Checked : Qt::Unchecked;

    if (role == Qt::DisplayRole)
    {
        switch (col)
        {
        case ColType:
            return typeText(d.type);
        case ColPath:
            return d.relPath;
        case ColSizeA:
            return (d.meta_A.isDir && d.type == DiffType::DiffType_OnlyA) ? QStringLiteral("[目录]") : (d.meta_A.size >= 0 ? formatFileSize(d.meta_A.size) : QStringLiteral("-"));
        case ColMtimeA:
            return d.meta_A.mtime > 0 ? formatMtime(d.meta_A.mtime) : QStringLiteral("-");
        case ColSizeB:
            return (d.meta_B.isDir && d.type == DiffType::DiffType_OnlyB) ? QStringLiteral("[目录]") : (d.meta_B.size >= 0 ? formatFileSize(d.meta_B.size) : QStringLiteral("-"));
        case ColMtimeB:
            return d.meta_B.mtime > 0 ? formatMtime(d.meta_B.mtime) : QStringLiteral("-");
        case ColDirection:
            return dirText(d.dir);
        default: break;
        }
    }

    if (role == Qt::TextAlignmentRole)
    {
        if (col == ColSizeA || col == ColSizeB)
            return int(Qt::AlignRight | Qt::AlignVCenter);
        if (col == ColChecked)
            return int(Qt::AlignCenter);
    }

    if (role == Qt::ForegroundRole && col == ColType)
        return typeColor(d.type);

    if (role == ChipColorRole && col == ColType)
        return typeColor(d.type);

    if (role == Qt::BackgroundRole && d.type == DiffType::DiffType_Conflict)
        return QColor(0xfff0ef);

    if (role == Qt::ToolTipRole)
    {
        QString tip = QStringLiteral("路径：%1\n类型：%2\nA 大小：%3\nA 修改时间：%4\nB 大小：%5\nB 修改时间：%6")
                .arg(d.relPath, typeText(d.type), formatFileSize(d.meta_A.size), formatMtime(d.meta_A.mtime),
                     formatFileSize(d.meta_B.size), formatMtime(d.meta_B.mtime));
        if (d.meta_A.hashValid || d.meta_B.hashValid)
            tip += QStringLiteral("\nA 哈希：%1\nB 哈希：%2").arg(d.meta_A.hash, d.meta_B.hash);
        if (!d.note.isEmpty())
            tip += QStringLiteral("\n说明：%1").arg(d.note);
        return tip;
    }

    return QVariant();
}

QVariant DiffTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
    {
        switch (section)
        {
        case ColChecked:
            return QStringLiteral("选择");
        case ColType:
            return QStringLiteral("状态");
        case ColPath:
            return QStringLiteral("相对路径");
        case ColSizeA:
            return QStringLiteral("A 大小");
        case ColMtimeA:
            return QStringLiteral("A 修改时间");
        case ColSizeB:
            return QStringLiteral("B 大小");
        case ColMtimeB:
            return QStringLiteral("B 修改时间");
        case ColDirection:
            return QStringLiteral("建议操作");
        }
    }
    return QAbstractTableModel::headerData(section, orientation, role);
}

Qt::ItemFlags DiffTableModel::flags(const QModelIndex& index) const
{
    if (!index.isValid())
        return Qt::NoItemFlags;
    Qt::ItemFlags f = Qt::ItemIsEnabled | Qt::ItemIsSelectable;
    if (index.column() == ColChecked)
        f |= Qt::ItemIsUserCheckable;
    return f;
}

bool DiffTableModel::setData(const QModelIndex& index, const QVariant& value, int role)
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_items.size())
        return false;

    if (role == Qt::CheckStateRole && index.column() == ColChecked)
    {
        m_items[index.row()].checked = (value.toInt() == Qt::Checked);
        emit dataChanged(index, index);
        return true;
    }
    return false;
}

QString DiffTableModel::typeText(DiffType t)
{
    switch (t)
    {
    case DiffType::DiffType_Same:
        return QStringLiteral("相同");
    case DiffType::DiffType_OnlyA:
        return QStringLiteral("仅 A 有");
    case DiffType::DiffType_OnlyB:
        return QStringLiteral("仅 B 有");
    case DiffType::DiffType_ContentDiff:
        return QStringLiteral("内容不同");
    case DiffType::DiffType_Conflict:
        return QStringLiteral("冲突");
    }
    return QString();
}

QString DiffTableModel::dirText(SyncSuggDirection d)
{
    switch (d)
    {
    case SyncSuggDirection::SyncSuggDirection_None:
        return QStringLiteral("跳过");
    case SyncSuggDirection::SyncSuggDirection_AtoB:
        return QStringLiteral("A → B");
    case SyncSuggDirection::SyncSuggDirection_BtoA:
        return QStringLiteral("B → A");
    case SyncSuggDirection::SyncSuggDirection_RemoveInB:
        return QStringLiteral("删除 B");
    case SyncSuggDirection::SyncSuggDirection_RemoveInA:
        return QStringLiteral("删除 A");
    }
    return QString();
}

// DiffFilterProxy
DiffFilterProxy::DiffFilterProxy(QObject* parent)
    : QSortFilterProxyModel(parent)
{
}

void DiffFilterProxy::setTypeFilter(int type)
{
    m_typeFilter = type;
    invalidateFilter();
}

void DiffFilterProxy::setSearchText(const QString& text)
{
    m_searchText = text;
    invalidateFilter();
}

bool DiffFilterProxy::filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const
{
    Q_UNUSED(sourceParent);
    const DiffTableModel* m = qobject_cast<const DiffTableModel*>(sourceModel());
    if (!m)
        return true;

    //const 引用直接访问，避免按值拷贝
    const DiffItemList& items = m->items();
    if (sourceRow < 0 || sourceRow >= items.size())
        return false;
    const FileDiffItem& item = items.at(sourceRow);

    if (m_typeFilter >= 0 && int(item.type) != m_typeFilter)
        return false;
    if (!m_searchText.isEmpty() && !item.relPath.contains(m_searchText, Qt::CaseInsensitive))
        return false;
    return true;
}
