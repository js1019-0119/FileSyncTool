#ifndef LOGTABLEMODEL_H
#define LOGTABLEMODEL_H

#include <QAbstractTableModel>
#include <QSortFilterProxyModel>
#include <QList>
#include <QDateTime>
#include "logdata.h"

//日志条目
struct LogItem
{
    QDateTime time;
    QString msg;
    SYSTEMLOGTYPE type = SYSTEMLOGTYPE::SYSTEMLOGTYPE_INFORMATION;
};

//日志面板 Model
class LogTableModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    enum Column {
        ColTime = 0,
        ColType,
        ColMsg,
        ColumnCount
    };

    explicit LogTableModel(QObject* parent = nullptr);

    void appendLog(const QString& msg, QDateTime time, SYSTEMLOGTYPE type);
    void clear();

    //导出日志到文本文件
    bool exportToFile(const QString& filePath, QString* error = nullptr) const;
    //获取指定行日志类型（供过滤代理使用）
    SYSTEMLOGTYPE typeAt(int row) const;

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    int columnCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;

    static QString typeText(SYSTEMLOGTYPE t);

private:
    QList<LogItem> m_items;
    static const int kMaxItems = 5000;   //上限，防止内存无限增长
};

//日志类型过滤代理
class LogFilterProxy : public QSortFilterProxyModel
{
    Q_OBJECT
public:
    explicit LogFilterProxy(QObject* parent = nullptr);

    void setTypeFilter(int type);        //-1 全部，否则为 SYSTEMLOGTYPE 枚举值

protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const override;

private:
    int m_typeFilter = -1;
};

#endif // LOGTABLEMODEL_H
