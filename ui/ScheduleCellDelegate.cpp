#include "ScheduleCellDelegate.h"

#include <QApplication>
#include <QPainter>
#include <QStyle>
#include <algorithm>
#include <cmath>

ScheduleCellDelegate::ScheduleCellDelegate(QObject* parent)
    : QStyledItemDelegate(parent) {
}

void ScheduleCellDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option,
                                 const QModelIndex& index) const {
    if (!index.isValid()) {
        QStyledItemDelegate::paint(painter, option, index);
        return;
    }

    if (index.column() == 0) {
        QStyledItemDelegate::paint(painter, option, index);
        return;
    }

    QStyleOptionViewItem opt(option);
    initStyleOption(&opt, index);

    QString text = opt.text;
    opt.text.clear();

    QStyleOptionViewItem baseOpt(opt);
    baseOpt.backgroundBrush = opt.palette.base();
    baseOpt.text.clear();
    baseOpt.icon = QIcon();
    QStyle* style = opt.widget ? opt.widget->style() : QApplication::style();
    style->drawControl(QStyle::CE_ItemViewItem, &baseOpt, painter, opt.widget);

    const int startMinute = index.data(ScheduleRoles::StartMinuteRole).toInt();
    const int durationMinutes = index.data(ScheduleRoles::DurationMinutesRole).toInt();
    const int spanRows = std::max(1, index.data(ScheduleRoles::SpanRowsRole).toInt());

    if (durationMinutes <= 0 || text.isEmpty()) {
        return;
    }

    const double pixelsPerMinute = option.rect.height() / (spanRows * 60.0);
    int startOffset = static_cast<int>(std::round(startMinute * pixelsPerMinute));
    int height = static_cast<int>(std::round(durationMinutes * pixelsPerMinute));
    startOffset = std::clamp(startOffset, 0, option.rect.height());
    height = std::min(height, option.rect.height() - startOffset);

    QRect eventRect = option.rect.adjusted(2, startOffset + 2, -2, -2);
    eventRect.setHeight(std::max(2, height - 4));

    QColor fillColor = index.data(ScheduleRoles::ColorRole).value<QColor>();
    if (!fillColor.isValid()) {
        fillColor = opt.palette.highlight().color().lighter(130);
    }
    if (option.state & QStyle::State_Selected) {
        fillColor = opt.palette.highlight().color();
    }

    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, true);
    painter->setBrush(fillColor);
    painter->setPen(Qt::NoPen);
    painter->drawRoundedRect(eventRect, 4, 4);
    painter->restore();

    painter->save();
    painter->setPen(opt.palette.text().color());
    QRect textRect = eventRect.adjusted(4, 2, -4, 0);
    painter->drawText(textRect, Qt::AlignLeft | Qt::AlignTop, text);
    painter->restore();

    painter->save();
    painter->setPen(QColor("#e0e0e0"));
    painter->drawLine(option.rect.left(), option.rect.center().y(), option.rect.right(), option.rect.center().y());
    painter->restore();
}
