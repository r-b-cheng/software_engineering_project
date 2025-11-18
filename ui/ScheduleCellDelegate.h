#ifndef SCHEDULECELLDELEGATE_H
#define SCHEDULECELLDELEGATE_H

#include <QStyledItemDelegate>

namespace ScheduleRoles {
constexpr int StartMinuteRole = Qt::UserRole + 1;
constexpr int DurationMinutesRole = Qt::UserRole + 2;
constexpr int SpanRowsRole = Qt::UserRole + 3;
constexpr int ColorRole = Qt::UserRole + 4;
}

class ScheduleCellDelegate : public QStyledItemDelegate {
    Q_OBJECT
public:
    explicit ScheduleCellDelegate(QObject* parent = nullptr);
    void paint(QPainter* painter, const QStyleOptionViewItem& option,
               const QModelIndex& index) const override;
};

#endif // SCHEDULECELLDELEGATE_H
