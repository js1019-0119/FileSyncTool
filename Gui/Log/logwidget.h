#ifndef LOGWIDGET_H
#define LOGWIDGET_H

#include <QWidget>
#include <QDateTime>
#include "logdata.h"

namespace Ui {
class LogWidget;
}

class LogTableModel;
class LogFilterProxy;
class BadgeDelegate;
class QComboBox;

class LogWidget : public QWidget
{
    Q_OBJECT

public:
    explicit LogWidget(QWidget *parent = nullptr);
    ~LogWidget();

public slots:
    void appendLog(const QString& msg, QDateTime time, SYSTEMLOGTYPE type);
    void clearLog();

protected:
    void initGui();
    void initConnect();

protected slots:
    void cb_FilterChanged_slots(int index);
    void btn_ClearClicked_slots();
    void btn_ExportClicked_slots();

private:
    Ui::LogWidget *ui;
    LogTableModel* m_model = nullptr;
    LogFilterProxy* m_proxy = nullptr;
    BadgeDelegate* m_badgeDelegate = nullptr;
    QComboBox* m_filterCombo = nullptr;
};

#endif // LOGWIDGET_H
