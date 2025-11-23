#include "AddEventDialog.h"
#include "ui_AddEventDialog.h"

#include <QComboBox>
#include <QDate>
#include <QDateEdit>
#include <QDateTime>
#include <QTime>
#include <QVariant>

namespace {
constexpr int kDefaultDurationSeconds = 3600;
constexpr int kTimeIntervalMinutes = 15;

void populateTimeCombo(QComboBox* combo) {
    if (!combo) {
        return;
    }
    combo->clear();
    const int stepsPerDay = (24 * 60) / kTimeIntervalMinutes;
    for (int i = 0; i < stepsPerDay; ++i) {
        const int minutes = i * kTimeIntervalMinutes;
        const QTime time(minutes / 60, minutes % 60);
        combo->addItem(time.toString("hh:mm"), time);
    }
}

void setComboToRoundedTime(QComboBox* combo, const QTime& target) {
    if (!combo || combo->count() == 0 || !target.isValid()) {
        return;
    }
    int minutes = target.hour() * 60 + target.minute();
    int index = (minutes + kTimeIntervalMinutes / 2) / kTimeIntervalMinutes;
    if (index < 0) {
        index = 0;
    }
    if (index >= combo->count()) {
        index = combo->count() - 1;
    }
    combo->setCurrentIndex(index);
}

QTime timeFromCombo(const QComboBox* combo) {
    if (!combo || combo->currentIndex() < 0) {
        return QTime(0, 0);
    }
    QVariant data = combo->currentData();
    if (data.canConvert<QTime>()) {
        return qvariant_cast<QTime>(data);
    }
    return QTime::fromString(combo->currentText(), "hh:mm");
}

// 标签相关辅助函数
int getTagsFromUI(const Ui::AddEventDialog* ui) {
    int tags = 0;
    if (ui->midtermCheck->isChecked()) tags |= TAG_MIDTERM;
    if (ui->finalCheck->isChecked()) tags |= TAG_FINAL;
    if (ui->reviewCheck->isChecked()) tags |= TAG_REVIEW;
    if (ui->makeupCheck->isChecked()) tags |= TAG_MAKEUP;
    if (ui->preCheck->isChecked()) tags |= TAG_PRE;
    if (ui->urgentCheck->isChecked()) tags |= TAG_URGENT;
    if (ui->importantCheck->isChecked()) tags |= TAG_IMPORTANT;
    return tags;
}

void setUITags(Ui::AddEventDialog* ui, int tags) {
    ui->midtermCheck->setChecked((tags & TAG_MIDTERM) != 0);
    ui->finalCheck->setChecked((tags & TAG_FINAL) != 0);
    ui->reviewCheck->setChecked((tags & TAG_REVIEW) != 0);
    ui->makeupCheck->setChecked((tags & TAG_MAKEUP) != 0);
    ui->preCheck->setChecked((tags & TAG_PRE) != 0);
    ui->urgentCheck->setChecked((tags & TAG_URGENT) != 0);
    ui->importantCheck->setChecked((tags & TAG_IMPORTANT) != 0);
}
}  // namespace

AddEventDialog::AddEventDialog(QWidget* parent)
    : QDialog(parent)
    , ui(new Ui::AddEventDialog) {
    ui->setupUi(this);

    populateTimeCombo(ui->startTimeCombo);
    populateTimeCombo(ui->endTimeCombo);
    ui->startDateEdit->setCalendarPopup(true);
    ui->endDateEdit->setCalendarPopup(true);

    QDateTime now = QDateTime::currentDateTime();
    QDateTime later = now.addSecs(kDefaultDurationSeconds);
    ui->startDateEdit->setDate(now.date());
    ui->endDateEdit->setDate(later.date());
    setComboToRoundedTime(ui->startTimeCombo, now.time());
    setComboToRoundedTime(ui->endTimeCombo, later.time());

    // 确保初始进入对话框时所有标签均为未选中
    setUITags(ui, 0);
}

AddEventDialog::~AddEventDialog() {
    delete ui;
}

ScheduleEvent AddEventDialog::getEvent() const {
    QDateTime startDateTime(ui->startDateEdit->date(), timeFromCombo(ui->startTimeCombo));
    QDateTime endDateTime(ui->endDateEdit->date(), timeFromCombo(ui->endTimeCombo));

    if (endDateTime <= startDateTime) {
        endDateTime = startDateTime.addSecs(kDefaultDurationSeconds);
    }

    const int weekday = startDateTime.date().dayOfWeek();  // Qt中周一=1, 周日=7
    TimeSlot slot(
        std::chrono::system_clock::from_time_t(startDateTime.toSecsSinceEpoch()),
        std::chrono::system_clock::from_time_t(endDateTime.toSecsSinceEpoch()),
        ui->isCourseCheck->isChecked());

    int tags = getTagsFromUI(ui);

    ScheduleEvent event(
        0,
        ui->nameEdit->text().toStdString(),
        ui->locationEdit->text().toStdString(),
        ui->descriptionEdit->toPlainText().toStdString(),
        weekday,
        slot,
        tags);

    return event;
}

void AddEventDialog::setEvent(const ScheduleEvent& event) {
    ui->nameEdit->setText(QString::fromUtf8(event.getEventName().c_str()));
    ui->locationEdit->setText(QString::fromUtf8(event.getLocation().c_str()));
    ui->descriptionEdit->setPlainText(QString::fromUtf8(event.getDescription().c_str()));

    const auto startTime = std::chrono::system_clock::to_time_t(event.getTimeSlot().getStartTime());
    const auto endTime = std::chrono::system_clock::to_time_t(event.getTimeSlot().getEndTime());
    const QDateTime startDateTime = QDateTime::fromSecsSinceEpoch(startTime);
    const QDateTime endDateTime = QDateTime::fromSecsSinceEpoch(endTime);

    ui->startDateEdit->setDate(startDateTime.date());
    ui->endDateEdit->setDate(endDateTime.date());
    setComboToRoundedTime(ui->startTimeCombo, startDateTime.time());
    setComboToRoundedTime(ui->endTimeCombo, endDateTime.time());
    ui->isCourseCheck->setChecked(event.getTimeSlot().getIsCourse());
    
    // 设置标签（先清空，再根据事件标签设定）
    setUITags(ui, 0);
    setUITags(ui, event.getTags());
}

void AddEventDialog::clear() {
    ui->nameEdit->clear();
    ui->locationEdit->clear();
    ui->descriptionEdit->clear();

    QDateTime now = QDateTime::currentDateTime();
    QDateTime later = now.addSecs(kDefaultDurationSeconds);
    ui->startDateEdit->setDate(now.date());
    ui->endDateEdit->setDate(later.date());
    setComboToRoundedTime(ui->startTimeCombo, now.time());
    setComboToRoundedTime(ui->endTimeCombo, later.time());
    ui->isCourseCheck->setChecked(false);
    
    // 清除所有标签
    setUITags(ui, 0);
}

void AddEventDialog::presetTimeRange(const QDateTime& start, const QDateTime& end) {
    QDateTime startTime = start.isValid() ? start : QDateTime::currentDateTime();
    QDateTime endTime = end.isValid() ? end : startTime.addSecs(kDefaultDurationSeconds);

    if (endTime <= startTime) {
        endTime = startTime.addSecs(kDefaultDurationSeconds);
    }

    ui->startDateEdit->setDate(startTime.date());
    ui->endDateEdit->setDate(endTime.date());
    setComboToRoundedTime(ui->startTimeCombo, startTime.time());
    setComboToRoundedTime(ui->endTimeCombo, endTime.time());
}

void AddEventDialog::setTagEditOnly(bool enable) {
    // 只允许编辑标签，其余控件设为只读或禁用
    ui->nameEdit->setEnabled(!enable);
    ui->locationEdit->setEnabled(!enable);
    ui->descriptionEdit->setEnabled(!enable);
    ui->startDateEdit->setEnabled(!enable);
    ui->endDateEdit->setEnabled(!enable);
    ui->startTimeCombo->setEnabled(!enable);
    ui->endTimeCombo->setEnabled(!enable);
    ui->isCourseCheck->setEnabled(!enable);
}

int AddEventDialog::getSelectedTags() const {
    return getTagsFromUI(ui);
}
