#include "conflictdialog.h"
#include "fileformat.h"
#include <QLabel>
#include <QPushButton>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QGroupBox>
#include <QSizePolicy>

namespace
{
    //构建单侧信息组
    QGroupBox* makeSideGroup(const QString& title, qint64 size, qint64 mtime,
                             const QString& hash, QWidget* parent)
    {
        auto* group = new QGroupBox(title, parent);
        group->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

        auto* form = new QFormLayout(group);
        form->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
        form->setLabelAlignment(Qt::AlignRight | Qt::AlignVCenter);
        form->setHorizontalSpacing(10);

        form->addRow(QStringLiteral("大小："), new QLabel(formatFileSize(size), group));
        form->addRow(QStringLiteral("修改时间："), new QLabel(formatMtime(mtime), group));

        auto* hashLabel = new QLabel(hash.isEmpty() ? QStringLiteral("-") : hash, group);
        hashLabel->setWordWrap(true);
        hashLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
        hashLabel->setMinimumWidth(200);
        hashLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        form->addRow(QStringLiteral("哈希："), hashLabel);

        return group;
    }
}

ConflictDialog::ConflictDialog(const FileDiffItem& item, const QString& dirA, const QString& dirB, QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(QStringLiteral("解决冲突"));
    setMinimumWidth(640);

    auto* title = new QLabel(QStringLiteral("<b>两边都修改过的文件，请选择保留哪一边</b>"), this);
    auto* pathLabel = new QLabel(QStringLiteral("相对路径：%1").arg(item.relPath), this);
    pathLabel->setWordWrap(true);
    pathLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);

    //两侧信息
    QGroupBox* groupA = makeSideGroup(QStringLiteral("A 侧（%1）").arg(dirA),
                                      item.meta_A.size, item.meta_A.mtime, item.meta_A.hash, this);
    QGroupBox* groupB = makeSideGroup(QStringLiteral("B 侧（%1）").arg(dirB),
                                      item.meta_B.size, item.meta_B.mtime, item.meta_B.hash, this);

    auto* groups = new QHBoxLayout;
    groups->addWidget(groupA, 1);
    groups->addWidget(groupB, 1);

    //按钮（居中）
    auto* btnKeepA = new QPushButton(QStringLiteral("保留 A（复制 A → B）"), this);
    auto* btnKeepB = new QPushButton(QStringLiteral("保留 B（复制 B → A）"), this);
    auto* btnSkip  = new QPushButton(QStringLiteral("跳过"), this);

    connect(btnKeepA, &QPushButton::clicked, this, [this]()
    {
        m_chosen = SyncSuggDirection::SyncSuggDirection_AtoB; accept();
    });
    connect(btnKeepB, &QPushButton::clicked, this, [this]()
    {
        m_chosen = SyncSuggDirection::SyncSuggDirection_BtoA; accept();
    });
    connect(btnSkip,  &QPushButton::clicked, this, [this]()
    {
        m_chosen = SyncSuggDirection::SyncSuggDirection_None; accept();
    });

    auto* btns = new QHBoxLayout;
    btns->addStretch(1);
    btns->addWidget(btnKeepA);
    btns->addWidget(btnKeepB);
    btns->addWidget(btnSkip);
    btns->addStretch(1);

    auto* lay = new QVBoxLayout(this);
    lay->addWidget(title);
    lay->addWidget(pathLabel);
    lay->addLayout(groups, 1);
    lay->addLayout(btns);
}
