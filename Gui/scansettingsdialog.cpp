#include "scansettingsdialog.h"
#include "hashfactory.h"
#include "scandifftuidata.h"
#include "syncdata.h"
#include <QComboBox>
#include <QCheckBox>
#include <QPushButton>
#include <QFormLayout>
#include <QVBoxLayout>
#include <QDialogButtonBox>

ScanSettingsDialog::ScanSettingsDialog(const ScanOptions& current, QWidget* parent)
    : QDialog(parent), m_currentMode(current.mode)
{
    setWindowTitle(QStringLiteral("扫描设置"));
    setMinimumWidth(400);
    initGui(current);
}

void ScanSettingsDialog::initGui(const ScanOptions& current)
{
    //哈希算法
    m_hashCombo = new QComboBox(this);
    m_hashCombo->addItems(HashFactory::available());
    m_hashCombo->setCurrentText(current.hashlogo);

    //深度校验
    m_deepCheck = new QCheckBox(QStringLiteral("深度校验（始终计算内容哈希）"), this);
    m_deepCheck->setToolTip(CheckoutDepthTip);
    m_deepCheck->setChecked(current.forceHash);

    //列出相同文件
    m_commonCheck = new QCheckBox(QStringLiteral("列出相同文件"), this);
    m_commonCheck->setChecked(current.listSameFiles);

    //镜像删除
    m_mirrorCheck = new QCheckBox(QStringLiteral("镜像删除（单向模式下删除目标端多余文件）"), this);
    m_mirrorCheck->setChecked(current.mirrorDelete);

    //布局
    auto* form = new QFormLayout;
    form->addRow(QStringLiteral("哈希算法："), m_hashCombo);
    form->addRow(QString(), m_deepCheck);
    form->addRow(QString(), m_commonCheck);
    form->addRow(QString(), m_mirrorCheck);

    auto* btns = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    btns->button(QDialogButtonBox::Ok)->setText(QStringLiteral("确认"));
    btns->button(QDialogButtonBox::Cancel)->setText(QStringLiteral("取消"));
    connect(btns, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(btns, &QDialogButtonBox::rejected, this, &QDialog::reject);

    auto* lay = new QVBoxLayout(this);
    lay->addLayout(form);
    lay->addWidget(btns);
}

ScanOptions ScanSettingsDialog::options() const
{
    ScanOptions opt;
    opt.hashlogo      = m_hashCombo->currentText();
    opt.forceHash     = m_deepCheck->isChecked();
    opt.listSameFiles = m_commonCheck->isChecked();
    opt.mirrorDelete  = m_mirrorCheck->isChecked();
    opt.mode          = m_currentMode;   //同步模式不在此对话框修改
    return opt;
}
