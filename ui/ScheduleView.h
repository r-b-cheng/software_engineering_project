#ifndef SCHEDULEVIEW_H
#define SCHEDULEVIEW_H

#include <QWidget>
#include <QTableView>
#include <QStandardItemModel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QDateTime>
#include "../datastructure/ScheduleEvent.h"
#include <vector>
#include <set>

class ScheduleView : public QWidget {
    Q_OBJECT

private:
    QTableView* tableView;
    QStandardItemModel* model;
    QPushButton* prevWeekButton;
    QPushButton* nextWeekButton;
    QLabel* weekLabel;
    int currentWeekOffset;
    QLabel* hoverLabel;
    
    std::vector<ScheduleEvent> currentEvents;
    std::vector<std::vector<bool>> occupiedSlots;
    bool dragSelecting;
    QDateTime dragStartTime;
    int dragStartSlot;
    int dragStartColumn;
    std::vector<std::pair<QDate, QString>> weekHolidays;
    std::set<std::pair<int,int>> suppressedCourseWeeks;
    std::set<std::pair<int,int>> courseTagWeeks;

    void setupUI();
    void updateWeekLabel();
    QStringList getWeekHeaders();
    QDate getDateForColumn(int column) const;
    void beginSelection(const QPoint& pos);
    void finalizeSelection(const QPoint& pos);
    void updateHoverIndicator(const QPoint& pos, bool forceHide = false);
    bool computeTimeAtPosition(const QPoint& pos, QDateTime& time, int& slot, int& column) const;
    bool isRangeAvailable(int column, int startSlot, int endSlot) const;
    void updateHeaderTooltips();
    void adjustHeaderFontSize();

public:
    explicit ScheduleView(QWidget* parent = nullptr);
    ~ScheduleView();

    void setWeekOffset(int offset);
    void setSchedule(const std::vector<ScheduleEvent>& events);
    void setHolidays(const std::vector<std::pair<QDate, QString>>& holidays);
    void setSuppressedCourseWeeks(const std::set<std::pair<int,int>>& suppressed);
    void setCourseTagWeeks(const std::set<std::pair<int,int>>& courseTags);
    
    int getCurrentWeekOffset() const;

signals:
    void weekChanged(int newOffset);
    void eventDoubleClicked(int eventId);
    void deleteEventRequested(int eventId);
    void createEventRequested(const QDateTime& start, const QDateTime& end);

private slots:
    void onPrevWeekClicked();
    void onNextWeekClicked();
    void onCellDoubleClicked(const QModelIndex& index);
    void onContextMenuRequested(const QPoint& pos);
    void onHeaderDoubleClicked(int section);

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;
};

#endif // SCHEDULEVIEW_H

