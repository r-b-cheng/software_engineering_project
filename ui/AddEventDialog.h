#ifndef ADDEVENTDIALOG_H
#define ADDEVENTDIALOG_H

#include <QDialog>
#include "../datastructure/ScheduleEvent.h"
#include <QDateTime>

namespace Ui {
class AddEventDialog;
}

class AddEventDialog : public QDialog {
    Q_OBJECT

public:
    explicit AddEventDialog(QWidget* parent = nullptr);
    ~AddEventDialog();

    ScheduleEvent getEvent() const;
    void setEvent(const ScheduleEvent& event);
    void clear();
    void presetTimeRange(const QDateTime& start, const QDateTime& end);

    void setTagEditOnly(bool enable);
    int getSelectedTags() const;

private:
    Ui::AddEventDialog *ui;
};

#endif // ADDEVENTDIALOG_H
