#include "taskwidget.h"
#include "ui_taskwidget.h"
#include "tasktablemodel.h"
#include "fileformat.h"
#include "tablehelper.h"
#include <QProgressBar>
#include <QLabel>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QMenu>

TaskWidget::TaskWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::TaskWidget)
{
    ui->setupUi(this);
    initGui();
    initConnect();
}

TaskWidget::~TaskWidget()
{
    //保存用户拖动的列宽/顺序
    saveTableHeaderState(ui->tableV_task, QStringLiteral("task/header"));
    delete ui;
}

void TaskWidget::initGui()
{
    //按钮文案（"全部开始"实为恢复全部）
    ui->pushButton_3->setText(QStringLiteral("全部恢复"));

    //任务表
    m_model = new TaskTableModel(this);
    m_delegate = new TaskProgressDelegate(this);
    ui->tableV_task->setModel(m_model);
    ui->tableV_task->setItemDelegateForColumn(TaskTableModel::ColProgress, m_delegate);
    ui->tableV_task->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->tableV_task->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->tableV_task->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->tableV_task->setAlternatingRowColors(true);
    ui->tableV_task->setShowGrid(false);
    ui->tableV_task->setTextElideMode(Qt::ElideMiddle);
    ui->tableV_task->verticalHeader()->setVisible(false);
    ui->tableV_task->verticalHeader()->setDefaultSectionSize(28);
    ui->tableV_task->horizontalHeader()->setStretchLastSection(false);
    ui->tableV_task->horizontalHeader()->setSectionsMovable(true);
    ui->tableV_task->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
    ui->tableV_task->horizontalHeader()->setSectionResizeMode(TaskTableModel::ColPath, QHeaderView::Stretch);
    ui->tableV_task->setColumnWidth(TaskTableModel::ColId, 40);
    ui->tableV_task->setColumnWidth(TaskTableModel::ColSize, 80);
    ui->tableV_task->setColumnWidth(TaskTableModel::ColDone, 80);
    ui->tableV_task->setColumnWidth(TaskTableModel::ColProgress, 160);
    ui->tableV_task->setColumnWidth(TaskTableModel::ColState, 70);
    //恢复用户拖动的列宽/顺序
    restoreTableHeaderState(ui->tableV_task, QStringLiteral("task/header"));

    //总体进度
    m_overallBar = new QProgressBar(ui->wd_process);
    m_overallBar->setRange(0, 100);
    m_overallBar->setValue(0);
    m_overallBar->setTextVisible(true);
    m_overallBar->setFormat(QStringLiteral("总进度 %p%"));

    m_overallLabel = new QLabel(QStringLiteral("就绪"), ui->wd_process);

    auto* lay = new QHBoxLayout(ui->wd_process);
    lay->setContentsMargins(0, 0, 0, 0);
    lay->addWidget(m_overallBar, 1);
    lay->addWidget(m_overallLabel);
}

void TaskWidget::initConnect()
{
    connect(ui->pushButton_3, &QPushButton::clicked, this, &TaskWidget::StartAllClicked);
    connect(ui->btn_stop, &QPushButton::clicked, this, &TaskWidget::PauseAllClicked);
    connect(ui->btn_cancel, &QPushButton::clicked, this, &TaskWidget::CancelAllClicked);

    //右键菜单
    ui->tableV_task->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->tableV_task, &QTableView::customContextMenuRequested,
            this, &TaskWidget::table_ContextMenu_slots);
}

void TaskWidget::table_ContextMenu_slots(const QPoint& pos)
{
    const QModelIndex idx = ui->tableV_task->indexAt(pos);
    if (!idx.isValid())
        return;

    const SyncTaskPtr task = m_model->taskAt(idx.row());
    if (task.isNull())
        return;

    QMenu menu(this);
    QAction* pauseAct = nullptr;
    QAction* resumeAct = nullptr;
    QAction* cancelAct = nullptr;

    if (task->state == TaskState::TaskState_Running || task->state == TaskState::TaskState_Pending)
        pauseAct = menu.addAction(QStringLiteral("暂停此任务"));
    if (task->state == TaskState::TaskState_Paused)
        resumeAct = menu.addAction(QStringLiteral("继续此任务"));
    if (!SyncTask::isFinished(task->state))
        cancelAct = menu.addAction(QStringLiteral("取消此任务"));

    QAction* chosen = menu.exec(ui->tableV_task->viewport()->mapToGlobal(pos));
    if (!chosen)
        return;

    if (chosen == pauseAct)
        emit PauseTask(task->id);
    else if (chosen == resumeAct)
        emit ResumeTask(task->id);
    else if (chosen == cancelAct)
        emit CancelTask(task->id);
}

void TaskWidget::setTasks(const QList<SyncTaskPtr>& tasks)
{
    m_model->setTasks(tasks);
}

void TaskWidget::updateTask(int id)
{
    m_model->updateTask(id);
}

void TaskWidget::setOverallProgress(qint64 done, qint64 total)
{
    if (total > 0)
        m_overallBar->setValue(int(done * 100 / total));
    m_overallLabel->setText(QStringLiteral("%1 / %2").arg(formatFileSize(done)).arg(formatFileSize(total)));
}

void TaskWidget::setOverallText(const QString& text)
{
    m_overallLabel->setText(text);
}
