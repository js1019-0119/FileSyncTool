#include "badgedelegate.h"
#include <QPainter>

BadgeDelegate::BadgeDelegate(QObject* parent)
    : QStyledItemDelegate(parent)
{
}

void BadgeDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option,
                          const QModelIndex& index) const
{
    const QVariant cv = index.data(ChipColorRole);
    if (!cv.isValid())
    {
        QStyledItemDelegate::paint(painter, option, index);
        return;
    }

    const QColor color = cv.value<QColor>();
    const QString text = index.data(Qt::DisplayRole).toString();

    painter->save();
    painter->setRenderHint(QPainter::Antialiasing);

    //浅色底 + 彩色描边的圆角
    const QRectF r = option.rect.adjusted(6, 4, -6, -4);
    QColor bg = color;
    bg.setAlpha(28);
    painter->setBrush(bg);
    painter->setPen(QPen(color, 1));
    painter->drawRoundedRect(r, 8, 8);

    //彩色加粗文字
    painter->setPen(color);
    QFont f = option.font;
    f.setBold(true);
    painter->setFont(f);
    painter->drawText(r, Qt::AlignCenter, text);
    painter->restore();
}
