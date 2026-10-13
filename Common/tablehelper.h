#ifndef TABLEHELPER_H
#define TABLEHELPER_H

#include <QTableView>
#include <QHeaderView>
#include <QSettings>

//保存表格列宽/顺序
inline void saveTableHeaderState(QTableView* view, const QString& key)
{
    if (!view || !view->horizontalHeader())
        return;
    QSettings settings;
    settings.setValue(key, view->horizontalHeader()->saveState());
}

//恢复表格列宽/顺序
inline void restoreTableHeaderState(QTableView* view, const QString& key)
{
    if (!view || !view->horizontalHeader())
        return;
    QSettings settings;
    const QByteArray state = settings.value(key).toByteArray();
    if (!state.isEmpty())
        view->horizontalHeader()->restoreState(state);
}

#endif // TABLEHELPER_H
