#include "logtablemodel.h"
#include "badgedelegate.h"
#include <QColor>
#include <QFile>
#include <QTextStream>

namespace
{
    //日志类型对应的主题色
    QColor typeColor(SYSTEMLOGTYPE t)
    {
        switch (t)
        {
        case SYSTEMLOGTYPE::SYSTEMLOGTYPE_INFORMATION:
            return QColor(0x188038);
        case SYSTEMLOGTYPE::SYSTEMLOGTYPE_WARRING:
            return QColor(0xe37400);
        case SYSTEMLOGTYPE::SYSTEMLOGTYPE_ERRO:
            return QColor(0xd93025);
        }
        return QColor(0x188038);
    }
}

LogTableModel::LogTableModel(QObject* parent)
    : QAbstractTableModel(parent)
{
}

void LogTableModel::appendLog(const QString& msg, QDateTime time, SYSTEMLOGTYPE type)
{
    LogItem item;
    item.time = time;
    item.msg = msg;
    item.type = type;

    beginInsertRows(QModelIndex(), m_items.size(), m_items.size());
    m_items.append(item);
    endInsertRows();

    //上限保护（每次只增一条，超出时移除最旧一条）
    if (m_items.size() > kMaxItems)
    {
        beginRemoveRows(QModelIndex(), 0, 0);
        m_items.removeFirst();
        endRemoveRows();
    }
}

void LogTableModel::clear()
{
    beginResetModel();
    m_items.clear();
    endResetModel();
}

bool LogTableModel::exportToFile(const QString& filePath, QString* error) const
{
    QFile f(filePath);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text))
    {
        if (error)
            *error = f.errorString();
        return false;
    }

    QTextStream ts(&f);
    ts.setCodec("UTF-8");
    ts << "时间\t类型\t内容\n";
    for (const LogItem& item : m_items)
    {
        ts << item.time.toString(QStringLiteral("yyyy-MM-dd HH:mm:ss")) << '\t'
           << typeText(item.type) << '\t'
           << item.msg << '\n';
    }
    f.close();
    return true;
}

SYSTEMLOGTYPE LogTableModel::typeAt(int row) const
{
    if (row < 0 || row >= m_items.size())
        return SYSTEMLOGTYPE::SYSTEMLOGTYPE_INFORMATION;
    return m_items.at(row).type;
}

int LogTableModel::rowCount(const QModelIndex& parent) const
{
    if (parent.isValid())
        return 0;
    return m_items.size();
}

int LogTableModel::columnCount(const QModelIndex& parent) const
{
    if (parent.isValid())
        return 0;
    return ColumnCount;
}

QVariant LogTableModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_items.size())
        return QVariant();

    const LogItem& item = m_items.at(index.row());
    const int col = index.column();

    if (role == Qt::DisplayRole)
    {
        switch (col)
        {
        case ColTime: return item.time.toString(QStringLiteral("HH:mm:ss"));
        case ColType: return typeText(item.type);
        case ColMsg:  return item.msg;
        default: break;
        }
    }

    if (role == Qt::ForegroundRole && col == ColType)
        return typeColor(item.type);

    if (role == ChipColorRole && col == ColType)
        return typeColor(item.type);

    if (role == Qt::ToolTipRole)
    {
        return QStringLiteral("[%1] %2").arg(item.time.toString(QStringLiteral("yyyy-MM-dd HH:mm:ss")), item.msg);
    }

    return QVariant();
}

QVariant LogTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
    {
        switch (section)
        {
        case ColTime: return QStringLiteral("时间");
        case ColType: return QStringLiteral("类型");
        case ColMsg:  return QStringLiteral("内容");
        }
    }
    return QAbstractTableModel::headerData(section, orientation, role);
}

QString LogTableModel::typeText(SYSTEMLOGTYPE t)
{
    switch (t)
    {
    case SYSTEMLOGTYPE::SYSTEMLOGTYPE_INFORMATION: return QStringLiteral("信息");
    case SYSTEMLOGTYPE::SYSTEMLOGTYPE_WARRING:     return QStringLiteral("警告");
    case SYSTEMLOGTYPE::SYSTEMLOGTYPE_ERRO:        return QStringLiteral("错误");
    }
    return QString();
}

// LogFilterProxy
LogFilterProxy::LogFilterProxy(QObject* parent)
    : QSortFilterProxyModel(parent)
{
}

void LogFilterProxy::setTypeFilter(int type)
{
    m_typeFilter = type;
    invalidateFilter();
}

bool LogFilterProxy::filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const
{
    Q_UNUSED(sourceParent);
    const LogTableModel* m = qobject_cast<const LogTableModel*>(sourceModel());
    if (!m)
        return true;
    if (m_typeFilter < 0)
        return true;
    return int(m->typeAt(sourceRow)) == m_typeFilter;
}
