#ifndef DIFFTABLEMODEL_H
#define DIFFTABLEMODEL_H

#include <QAbstractTableModel>
#include <QSortFilterProxyModel>
#include "scandata.h"

//差异列表 Model：支持勾选、按状态着色
class DiffTableModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    enum Column {
        ColChecked = 0,
        ColType,
        ColPath,
        ColSizeA,
        ColMtimeA,
        ColSizeB,
        ColMtimeB,
        ColDirection,
        ColumnCount
    };

    explicit DiffTableModel(QObject* parent = nullptr);

    void setItems(const DiffItemList& items);
    const DiffItemList& items() const { return m_items; }
    FileDiffItem itemAt(int row) const;
    void setChecked(int row, bool checked);
    void setDirection(int row, SyncSuggDirection dir);

    //批量操作
    void setAllChecked(bool checked);
    void invertChecked();

    int countByType(DiffType t) const;
    int selectedCount() const;

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    int columnCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
    Qt::ItemFlags flags(const QModelIndex& index) const override;
    bool setData(const QModelIndex& index, const QVariant& value, int role) override;

    static QString typeText(DiffType t);
    static QString dirText(SyncSuggDirection d);

private:
    DiffItemList m_items;
};

//差异列表过滤代理：按状态过滤 + 文本搜索 + 排序
class DiffFilterProxy : public QSortFilterProxyModel
{
    Q_OBJECT
public:
    explicit DiffFilterProxy(QObject* parent = nullptr);

    void setTypeFilter(int type);        //-1 表示全部
    void setSearchText(const QString& text);

protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const override;

private:
    int m_typeFilter = -1;
    QString m_searchText;
};

#endif // DIFFTABLEMODEL_H
