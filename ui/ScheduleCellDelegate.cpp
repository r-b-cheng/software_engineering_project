#include "ScheduleCellDelegate.h"
#include "../datastructure/ScheduleEvent.h"

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
        fillColor = QColor("#d3d3d3");
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

    // 始终优先绘制事件名称（第一行）
    QFont nameFont = opt.font;
    painter->setFont(nameFont);
    QFontMetrics nameMetrics(nameFont);
    int nameHeight = nameMetrics.height();
    QRect nameRect = textRect;
    nameRect.setHeight(nameHeight);
    painter->drawText(nameRect, Qt::AlignLeft | Qt::AlignTop, text);

    // 标签以小字体绘制在事件名之后（第二行）
    const int tags = index.data(ScheduleRoles::TagsRole).toInt();
    if (tags != 0) {
        QStringList tagNames;
        if (tags & TAG_MIDTERM) tagNames << QString::fromUtf8("期中");
        if (tags & TAG_FINAL) tagNames << QString::fromUtf8("期末");
        if (tags & TAG_REVIEW) tagNames << QString::fromUtf8("复习");
        if (tags & TAG_MAKEUP) tagNames << QString::fromUtf8("补课");
        if (tags & TAG_PRE) tagNames << QString::fromUtf8("Pre");
        if (tags & TAG_URGENT) tagNames << QString::fromUtf8("紧急");
        if (tags & TAG_IMPORTANT) tagNames << QString::fromUtf8("重要");

        if (!tagNames.isEmpty()) {
            QString tagsText = tagNames.join(QString::fromUtf8(" "));
            QFont tagFont = opt.font;
            tagFont.setPointSize(std::max(6, tagFont.pointSize() - 2));
            painter->setFont(tagFont);
            QFontMetrics tagMetrics(tagFont);
            int tagHeight = tagMetrics.height();

            QRect tagRect = textRect;
            tagRect.setTop(nameRect.bottom() + 2);
            tagRect.setHeight(tagHeight);
            painter->setPen(QColor(150, 150, 150));
            painter->drawText(tagRect, Qt::AlignLeft | Qt::AlignTop, tagsText);
        }
    }

    painter->restore();

    painter->save();
    painter->setPen(QColor("#e0e0e0"));
    painter->drawLine(option.rect.left(), option.rect.center().y(), option.rect.right(), option.rect.center().y());
    painter->restore();
}
