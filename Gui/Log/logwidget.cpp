#include "logwidget.h"
#include "ui_logwidget.h"
#include "logtablemodel.h"
#include "badgedelegate.h"
#include "tablehelper.h"
#include <QHeaderView>
#include <QComboBox>
#include <QPushButton>
#include <QHBoxLayout>
#include <QScrollBar>
#include <QFileDialog>
#include <QMessageBox>

LogWidget::LogWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::LogWidget)
{
    ui->setupUi(this);
    initGui();
    initConnect();
}

LogWidget::~LogWidget()
{
    //保存用户拖动的列宽/顺序
    saveTableHeaderState(ui->tableV_Log, QStringLiteral("log/header"));
    delete ui;
}

void LogWidget::initGui()
{
    //日志表
    m_model = new LogTableModel(this);
    m_proxy = new LogFilterProxy(this);
    m_proxy->setSourceModel(m_model);
    m_badgeDelegate = new BadgeDelegate(this);
    ui->tableV_Log->setModel(m_proxy);
    ui->tableV_Log->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->tableV_Log->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->tableV_Log->setAlternatingRowColors(true);
    ui->tableV_Log->setShowGrid(false);
    ui->tableV_Log->setWordWrap(false);
    ui->tableV_Log->setTextElideMode(Qt::ElideMiddle);
    ui->tableV_Log->verticalHeader()->setVisible(false);
    ui->tableV_Log->verticalHeader()->setDefaultSectionSize(24);
    ui->tableV_Log->horizontalHeader()->setStretchLastSection(false);
    ui->tableV_Log->horizontalHeader()->setSectionsMovable(true);
    ui->tableV_Log->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
    ui->tableV_Log->horizontalHeader()->setSectionResizeMode(LogTableModel::ColMsg, QHeaderView::Stretch);
    ui->tableV_Log->setColumnWidth(LogTableModel::ColTime, 80);
    ui->tableV_Log->setColumnWidth(LogTableModel::ColType, 60);
    //类型列使用彩色代理
    ui->tableV_Log->setItemDelegateForColumn(LogTableModel::ColType, m_badgeDelegate);
    //恢复用户拖动的列宽/顺序
    restoreTableHeaderState(ui->tableV_Log, QStringLiteral("log/header"));

    //底部过滤/操作栏
    m_filterCombo = new QComboBox(ui->wd_page);
    m_filterCombo->addItem(QStringLiteral("全部类型"), -1);
    m_filterCombo->addItem(QStringLiteral("信息"), int(SYSTEMLOGTYPE::SYSTEMLOGTYPE_INFORMATION));
    m_filterCombo->addItem(QStringLiteral("警告"), int(SYSTEMLOGTYPE::SYSTEMLOGTYPE_WARRING));
    m_filterCombo->addItem(QStringLiteral("错误"), int(SYSTEMLOGTYPE::SYSTEMLOGTYPE_ERRO));

    auto* btnClear = new QPushButton(QStringLiteral("清空"), ui->wd_page);
    auto* btnExport = new QPushButton(QStringLiteral("导出"), ui->wd_page);

    auto* lay = new QHBoxLayout(ui->wd_page);
    lay->setContentsMargins(2, 2, 2, 2);
    lay->addWidget(m_filterCombo);
    lay->addStretch(1);
    lay->addWidget(btnClear);
    lay->addWidget(btnExport);

    connect(btnClear, &QPushButton::clicked, this, &LogWidget::btn_ClearClicked_slots);
    connect(btnExport, &QPushButton::clicked, this, &LogWidget::btn_ExportClicked_slots);
}

void LogWidget::initConnect()
{
    connect(m_filterCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &LogWidget::cb_FilterChanged_slots);
}

void LogWidget::appendLog(const QString& msg, QDateTime time, SYSTEMLOGTYPE type)
{
    //智能滚动：仅当用户已在底部时才自动跟随，避免打断向上翻阅
    QScrollBar* sb = ui->tableV_Log->verticalScrollBar();
    const bool atBottom = (sb ? sb->value() >= sb->maximum() - 4 : true);

    m_model->appendLog(msg, time, type);

    if (atBottom)
        ui->tableV_Log->scrollToBottom();
}

void LogWidget::clearLog()
{
    m_model->clear();
}

void LogWidget::cb_FilterChanged_slots(int index)
{
    Q_UNUSED(index);
    m_proxy->setTypeFilter(m_filterCombo->currentData().toInt());
}

void LogWidget::btn_ClearClicked_slots()
{
    clearLog();
}

void LogWidget::btn_ExportClicked_slots()
{
    const QString path = QFileDialog::getSaveFileName(this, QStringLiteral("导出日志"),
        QStringLiteral("sync_log.txt"), QStringLiteral("文本文件 (*.txt);;所有文件 (*)"));
    if (path.isEmpty())
        return;

    QString err;
    if (m_model->exportToFile(path, &err))
        QMessageBox::information(this, QStringLiteral("导出成功"), QStringLiteral("日志已导出到：\n%1").arg(path));
    else
        QMessageBox::warning(this, QStringLiteral("导出失败"), err);
}
