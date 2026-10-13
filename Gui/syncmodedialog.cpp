#include "syncmodedialog.h"
#include "scandifftuidata.h"
#include <QLabel>
#include <QRadioButton>
#include <QButtonGroup>
#include <QPushButton>
#include <QVBoxLayout>
#include <QDialogButtonBox>

SyncModeDialog::SyncModeDialog(SyncMode current, QWidget* parent)
    : QDialog(parent), m_mode(current)
{
    setWindowTitle(QStringLiteral("同步模式"));
    setMinimumWidth(340);

    auto* tip = new QLabel(QStringLiteral("请选择同步模式："), this);

    auto* rTwoWay = new QRadioButton(QStringLiteral("%1 — 两边互相镜像，两边都修改过的文件标记为冲突").arg(SyncModeTwoWay), this);
    auto* rAtoB   = new QRadioButton(SyncModeAtoB, this);
    auto* rBtoA   = new QRadioButton(SyncModeBtoA, this);

    rTwoWay->setChecked(m_mode == SyncMode::SyncMode_TwoWay);
    rAtoB->setChecked(m_mode == SyncMode::SyncMode_AtoB);
    rBtoA->setChecked(m_mode == SyncMode::SyncMode_BtoA);

    auto* grp = new QButtonGroup(this);
    grp->addButton(rTwoWay, int(SyncMode::SyncMode_TwoWay));
    grp->addButton(rAtoB, int(SyncMode::SyncMode_AtoB));
    grp->addButton(rBtoA, int(SyncMode::SyncMode_BtoA));

    auto* btns = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    btns->button(QDialogButtonBox::Ok)->setText(QStringLiteral("确认"));
    btns->button(QDialogButtonBox::Cancel)->setText(QStringLiteral("取消"));
    connect(btns, &QDialogButtonBox::accepted, this, [this, grp]() {
        m_mode = SyncMode(grp->checkedId());
        accept();
    });
    connect(btns, &QDialogButtonBox::rejected, this, &QDialog::reject);

    auto* lay = new QVBoxLayout(this);
    lay->addWidget(tip);
    lay->addWidget(rTwoWay);
    lay->addWidget(rAtoB);
    lay->addWidget(rBtoA);
    lay->addWidget(btns);
}
