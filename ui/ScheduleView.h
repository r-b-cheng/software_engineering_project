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

class ScheduleView : public QWidget {
    Q_OBJECT

private:
    QTableView* tableView;
    QStandardItemModel* model;
    QPushButton* prevWeekButton;
    QPushButton* nextWeekButton;
    QLabel* weekLabel;
    int currentWeekOffset;
    
    std::vector<ScheduleEvent> currentEvents;
    std::vector<std::vector<bool>> occupiedSlots;

    void setupUI();
    void updateWeekLabel();
    QStringList getWeekHeaders();
    QDate getDateForColumn(int column) const;
    void processSelectionRelease();
    bool tryBuildSelectionRange(QDateTime& start, QDateTime& end) const;

public:
    explicit ScheduleView(QWidget* parent = nullptr);
    ~ScheduleView();

    void setWeekOffset(int offset);
    void setSchedule(const std::vector<ScheduleEvent>& events);
    
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

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;
};

#endif // SCHEDULEVIEW_H

