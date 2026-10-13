#ifndef BADGEDELEGATE_H
#define BADGEDELEGATE_H

#include <QStyledItemDelegate>
#include <QColor>

//状态角色（用于状态/类型列）
static const int ChipColorRole = Qt::UserRole + 100;

//状态代理绘制
class BadgeDelegate : public QStyledItemDelegate
{
    Q_OBJECT
public:
    explicit BadgeDelegate(QObject* parent = nullptr);

    void paint(QPainter* painter, const QStyleOptionViewItem& option,
               const QModelIndex& index) const override;
};

#endif // BADGEDELEGATE_H
