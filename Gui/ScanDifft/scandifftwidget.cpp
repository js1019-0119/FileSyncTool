#include "scandifftwidget.h"
#include "ui_scandifftwidget.h"
#include "hashfactory.h"
#include "scandifftuidata.h"
#include "difftablemodel.h"
#include "conflictdialog.h"
#include "badgedelegate.h"
#include "tablehelper.h"
#include "syncdata.h"
#include "differentdata.h"
#include <QHeaderView>
#include <QLabel>
#include <QMenu>

ScanDifftWidget::ScanDifftWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ScanDifftWidget)
{
    ui->setupUi(this);
    initGui();
    initConnect();
}

ScanDifftWidget::~ScanDifftWidget()
{
    //保存用户拖动的列宽/顺序
    saveTableHeaderState(ui->tableV_Diffent, QStringLiteral("diff/header"));
    delete ui;
}

void ScanDifftWidget::setbtnScanText(const QString &text)
{
    ui->btn_scan->setText(text);
}

void ScanDifftWidget::setScanProgressValue(const int progress)
{
    ui->wd_scan_process->setBusy(false);
    ui->wd_scan_process->setValue(progress);
}

void ScanDifftWidget::setScanProgressText(const QString &text)
{
    ui->wd_scan_process->setText(text);
}

void ScanDifftWidget::setScanBusy(const QString &text)
{
    ui->wd_scan_process->setBusy(true);
    ui->wd_scan_process->setText(text);
}

void ScanDifftWidget::resetScanProgress()
{
    ui->wd_scan_process->reset();
}

void ScanDifftWidget::setDiffItems(const DiffItemList &items)
{
    m_diffModel->setItems(items);
    updateStats();
}

DiffItemList ScanDifftWidget::getDiffItems() const
{
    return m_diffModel->items();
}

void ScanDifftWidget::setCompareDirs(const QString &dirA, const QString &dirB)
{
    m_dirA = dirA;
    m_dirB = dirB;
}

void ScanDifftWidget::applyScanOptions(const ScanOptions& options)
{
    //保留时间容差（设置对话框未暴露该字段）
    const qint64 tol = m_scanoptions.mtimeToleranceMs;
    m_scanoptions = options;
    m_scanoptions.mtimeToleranceMs = tol;

    //更新控件，并避免触发 ScanOptionsChanged 造成递归
    ui->cb_hash->blockSignals(true);
    ui->ck_deep->blockSignals(true);
    ui->ck_common->blockSignals(true);
    ui->ck_mirrodelete->blockSignals(true);
    ui->cb_sync_mode->blockSignals(true);

    ui->cb_hash->setCurrentText(options.hashlogo);
    ui->ck_deep->setChecked(options.forceHash);
    ui->ck_common->setChecked(options.listSameFiles);
    ui->ck_mirrodelete->setChecked(options.mirrorDelete);
    ui->cb_sync_mode->setCurrentIndex(int(options.mode));

    ui->cb_hash->blockSignals(false);
    ui->ck_deep->blockSignals(false);
    ui->ck_common->blockSignals(false);
    ui->ck_mirrodelete->blockSignals(false);
    ui->cb_sync_mode->blockSignals(false);

    emit ScanOptionsChanged(m_scanoptions);
}

void ScanDifftWidget::initGui()
{
    ui->cb_hash->addItems(HashFactory::available());
    ui->cb_hash->setCurrentText(HashFactory::defaultAlgorithm());

    ui->ck_deep->setToolTip(CheckoutDepthTip);
    ui->ck_deep->setCheckState(Qt::Unchecked);

    ui->ck_common->setToolTip(QStringLiteral("列出两边内容相同的文件（默认隐藏）"));
    ui->ck_common->setCheckState(Qt::Unchecked);

    ui->ck_mirrodelete->setToolTip(QStringLiteral("单向同步时删除目标端多余的文件/目录"));
    ui->ck_mirrodelete->setCheckState(Qt::Unchecked);

    ui->cb_sync_mode->addItem(SyncModeTwoWay,int(SyncMode::SyncMode_TwoWay));
    ui->cb_sync_mode->addItem(SyncModeAtoB,int(SyncMode::SyncMode_AtoB));
    ui->cb_sync_mode->addItem(SyncModeBtoA,int(SyncMode::SyncMode_BtoA));
    ui->cb_sync_mode->setCurrentText(SyncModeTwoWay);

    //差异列表
    m_diffModel = new DiffTableModel(this);
    m_diffProxy = new DiffFilterProxy(this);
    m_diffProxy->setSourceModel(m_diffModel);
    m_badgeDelegate = new BadgeDelegate(this);
    ui->tableV_Diffent->setModel(m_diffProxy);
    ui->tableV_Diffent->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->tableV_Diffent->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->tableV_Diffent->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->tableV_Diffent->setSortingEnabled(true);
    ui->tableV_Diffent->setAlternatingRowColors(true);
    ui->tableV_Diffent->setShowGrid(false);
    ui->tableV_Diffent->setTextElideMode(Qt::ElideMiddle);
    ui->tableV_Diffent->verticalHeader()->setVisible(false);
    ui->tableV_Diffent->verticalHeader()->setDefaultSectionSize(28);
    ui->tableV_Diffent->horizontalHeader()->setStretchLastSection(false);
    ui->tableV_Diffent->horizontalHeader()->setSectionsMovable(true);
    ui->tableV_Diffent->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
    ui->tableV_Diffent->horizontalHeader()->setSectionResizeMode(DiffTableModel::ColPath, QHeaderView::Stretch);
    ui->tableV_Diffent->setColumnWidth(DiffTableModel::ColChecked, 46);
    ui->tableV_Diffent->setColumnWidth(DiffTableModel::ColType, 90);
    ui->tableV_Diffent->setColumnWidth(DiffTableModel::ColSizeA, 90);
    ui->tableV_Diffent->setColumnWidth(DiffTableModel::ColMtimeA, 150);
    ui->tableV_Diffent->setColumnWidth(DiffTableModel::ColSizeB, 90);
    ui->tableV_Diffent->setColumnWidth(DiffTableModel::ColMtimeB, 150);
    ui->tableV_Diffent->setColumnWidth(DiffTableModel::ColDirection, 80);
    //状态列使用代理
    ui->tableV_Diffent->setItemDelegateForColumn(DiffTableModel::ColType, m_badgeDelegate);
    //恢复用户拖动的列宽/顺序
    restoreTableHeaderState(ui->tableV_Diffent, QStringLiteral("diff/header"));

    //筛选下拉框
    ui->cb_filter->addItem(QStringLiteral("全部"), -1);
    ui->cb_filter->addItem(DiffTableModel::typeText(DiffType::DiffType_OnlyA), int(DiffType::DiffType_OnlyA));
    ui->cb_filter->addItem(DiffTableModel::typeText(DiffType::DiffType_OnlyB), int(DiffType::DiffType_OnlyB));
    ui->cb_filter->addItem(DiffTableModel::typeText(DiffType::DiffType_ContentDiff), int(DiffType::DiffType_ContentDiff));
    ui->cb_filter->addItem(DiffTableModel::typeText(DiffType::DiffType_Conflict), int(DiffType::DiffType_Conflict));

    updateStats();
}

void ScanDifftWidget::initConnect()
{
    connect(ui->cb_hash,&QComboBox::currentTextChanged,this,&ScanDifftWidget::cb_HashTextChanged_Slots);
    connect(ui->ck_deep,&QCheckBox::stateChanged,this,&ScanDifftWidget::ck_DeepCheckStateChanged_Slots);
    connect(ui->ck_common,&QCheckBox::stateChanged,this,&ScanDifftWidget::ck_ListCommonCheckStateChanged_Slots);
    connect(ui->ck_mirrodelete,&QCheckBox::stateChanged,this,&ScanDifftWidget::ck_MirrorDeleteCheckStateChanged_Slots);
    connect(ui->cb_sync_mode,&QComboBox::currentTextChanged,this,&ScanDifftWidget::cb_SyncModeTextChanged_Slots);
    connect(ui->btn_scan,&QPushButton::clicked,this,&ScanDifftWidget::StartScan);

    //差异列表
    connect(ui->cb_filter, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &ScanDifftWidget::cb_FilterChanged_Slots);
    connect(ui->le_find, &QLineEdit::textChanged, this, &ScanDifftWidget::le_FindTextChanged_Slots);
    connect(ui->tableV_Diffent, &QTableView::doubleClicked, this, &ScanDifftWidget::table_DoubleClicked_Slots);
    ui->tableV_Diffent->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->tableV_Diffent, &QTableView::customContextMenuRequested,
            this, &ScanDifftWidget::table_ContextMenu_slots);

    //同步
    connect(ui->btn_sync, &QPushButton::clicked, this, &ScanDifftWidget::btn_SyncClicked_Slots);
    connect(ui->btn_conflict_handel, &QPushButton::clicked, this, &ScanDifftWidget::btn_ConflictHandleClicked_Slots);
}

void ScanDifftWidget::cb_HashTextChanged_Slots(const QString &text)
{
    m_scanoptions.hashlogo = text;
    emit ScanOptionsChanged(m_scanoptions);
}

void ScanDifftWidget::ck_DeepCheckStateChanged_Slots(int state)
{
    m_scanoptions.forceHash = Qt::Checked == state ? true : false;
    emit ScanOptionsChanged(m_scanoptions);
}

void ScanDifftWidget::ck_ListCommonCheckStateChanged_Slots(int state)
{
    m_scanoptions.listSameFiles = Qt::Checked == state ? true : false;
    emit ScanOptionsChanged(m_scanoptions);
}

void ScanDifftWidget::ck_MirrorDeleteCheckStateChanged_Slots(int state)
{
    m_scanoptions.mirrorDelete = Qt::Checked == state ? true : false;
    emit ScanOptionsChanged(m_scanoptions);
}

void ScanDifftWidget::cb_SyncModeTextChanged_Slots(const QString &text)
{
    Q_UNUSED(text);
    m_scanoptions.mode  = SyncMode(ui->cb_sync_mode->currentData().toInt());
    emit ScanOptionsChanged(m_scanoptions);
}

void ScanDifftWidget::btn_ScanClicked_Slots()
{
    emit StartScan();
}

void ScanDifftWidget::cb_FilterChanged_Slots(int index)
{
    Q_UNUSED(index);
    m_diffProxy->setTypeFilter(ui->cb_filter->currentData().toInt());
}

void ScanDifftWidget::le_FindTextChanged_Slots(const QString &text)
{
    m_diffProxy->setSearchText(text);
}

void ScanDifftWidget::btn_SyncClicked_Slots()
{
    emit StartSync();
}

void ScanDifftWidget::btn_ConflictHandleClicked_Slots()
{
    //优先处理选中的冲突行，否则处理第一个冲突
    const QModelIndex idx = ui->tableV_Diffent->currentIndex();
    if (idx.isValid())
    {
        const QModelIndex src = m_diffProxy->mapToSource(idx);
        if (m_diffModel->itemAt(src.row()).type == DiffType::DiffType_Conflict)
        {
            handleConflictAtRow(src.row());
            return;
        }
    }

    const DiffItemList& items = m_diffModel->items();
    for (int i = 0; i < items.size(); ++i)
    {
        if (items.at(i).type == DiffType::DiffType_Conflict)
        {
            handleConflictAtRow(i);
            return;
        }
    }
}

void ScanDifftWidget::table_DoubleClicked_Slots(const QModelIndex &index)
{
    if (!index.isValid())
        return;
    const QModelIndex src = m_diffProxy->mapToSource(index);
    if (m_diffModel->itemAt(src.row()).type == DiffType::DiffType_Conflict)
        handleConflictAtRow(src.row());
}

void ScanDifftWidget::table_ContextMenu_slots(const QPoint& pos)
{
    const QModelIndex idx = ui->tableV_Diffent->indexAt(pos);
    const bool hasRow = idx.isValid();
    const int srcRow = hasRow ? m_diffProxy->mapToSource(idx).row() : -1;

    QMenu menu(this);
    QAction* a2b = nullptr;
    QAction* b2a = nullptr;
    QAction* skip = nullptr;
    QAction* delB = nullptr;
    QAction* delA = nullptr;
    QAction* resolve = nullptr;

    if (hasRow)
    {
        const FileDiffItem item = m_diffModel->itemAt(srcRow);
        a2b = menu.addAction(QStringLiteral("复制 A → B"));
        b2a = menu.addAction(QStringLiteral("复制 B → A"));
        skip = menu.addAction(QStringLiteral("跳过"));
        if (item.type == DiffType::DiffType_OnlyB)
            delB = menu.addAction(QStringLiteral("删除 B"));
        if (item.type == DiffType::DiffType_OnlyA)
            delA = menu.addAction(QStringLiteral("删除 A"));
        if (item.type == DiffType::DiffType_Conflict)
        {
            menu.addSeparator();
            resolve = menu.addAction(QStringLiteral("解决冲突…"));
        }
        menu.addSeparator();
    }

    QAction* selAll  = menu.addAction(QStringLiteral("全选"));
    QAction* selNone = menu.addAction(QStringLiteral("全不选"));
    QAction* selInv  = menu.addAction(QStringLiteral("反选"));

    QAction* chosen = menu.exec(ui->tableV_Diffent->viewport()->mapToGlobal(pos));
    if (!chosen)
        return;

    if (chosen == a2b)
        m_diffModel->setDirection(srcRow, SyncSuggDirection::SyncSuggDirection_AtoB);
    else if (chosen == b2a)
        m_diffModel->setDirection(srcRow, SyncSuggDirection::SyncSuggDirection_BtoA);
    else if (chosen == skip)
        m_diffModel->setDirection(srcRow, SyncSuggDirection::SyncSuggDirection_None);
    else if (chosen == delB)
        m_diffModel->setDirection(srcRow, SyncSuggDirection::SyncSuggDirection_RemoveInB);
    else if (chosen == delA)
        m_diffModel->setDirection(srcRow, SyncSuggDirection::SyncSuggDirection_RemoveInA);
    else if (chosen == resolve)
        handleConflictAtRow(srcRow);
    else if (chosen == selAll)
        m_diffModel->setAllChecked(true);
    else if (chosen == selNone)
        m_diffModel->setAllChecked(false);
    else if (chosen == selInv)
        m_diffModel->invertChecked();

    updateStats();
}

void ScanDifftWidget::handleConflictAtRow(int row)
{
    const FileDiffItem item = m_diffModel->itemAt(row);
    if (item.type != DiffType::DiffType_Conflict)
        return;

    ConflictDialog dlg(item, m_dirA, m_dirB, this);
    if (dlg.exec() == QDialog::Accepted)
        m_diffModel->setDirection(row, dlg.chosen());
    updateStats();
}

void ScanDifftWidget::updateStats()
{
    const DiffItemList& items = m_diffModel->items();
    const int sel = m_diffModel->selectedCount();
    const int conflict = m_diffModel->countByType(DiffType::DiffType_Conflict);
    ui->groupBox->setTitle(QStringLiteral("差异列表（共 %1 项，已选 %2 项，冲突 %3 项）")
                           .arg(items.size()).arg(sel).arg(conflict));
}
