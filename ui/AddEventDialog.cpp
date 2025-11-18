#include "AddEventDialog.h"
#include "ui_AddEventDialog.h"
#include <QDateTime>
#include <QDate>
#include <QTime>

AddEventDialog::AddEventDialog(QWidget* parent)
    : QDialog(parent)
    , ui(new Ui::AddEventDialog) {
    ui->setupUi(this);
    
    // 设置默认时间（使用当前时间）
    ui->startTimeEdit->setDateTime(QDateTime::currentDateTime());
    ui->endTimeEdit->setDateTime(QDateTime::currentDateTime().addSecs(3600));
}

AddEventDialog::~AddEventDialog() {
    delete ui;
}

ScheduleEvent AddEventDialog::getEvent() const {
    // 获取用户选择的完整日期时间
    QDateTime startDateTime = ui->startTimeEdit->dateTime();
    QDateTime endDateTime = ui->endTimeEdit->dateTime();
    
    // 确保结束时间在开始时间之后
    if (endDateTime <= startDateTime) {
        endDateTime = startDateTime.addSecs(3600); // 默认1小时
    }
    
    // 根据开始时间的日期自动计算星期
    int weekday = startDateTime.date().dayOfWeek();  // Qt中周一=1, 周日=7
    
    // 创建时间段
    TimeSlot slot(
        std::chrono::system_clock::from_time_t(startDateTime.toSecsSinceEpoch()),
        std::chrono::system_clock::from_time_t(endDateTime.toSecsSinceEpoch()),
        ui->isCourseCheck->isChecked()
    );

    // 创建事件（ID将由调用者设置）
    ScheduleEvent event(
        0,
        ui->nameEdit->text().toStdString(),
        ui->locationEdit->text().toStdString(),
        ui->descriptionEdit->toPlainText().toStdString(),
        weekday,  // 使用自动计算的星期
        slot
    );

    return event;
}

void AddEventDialog::setEvent(const ScheduleEvent& event) {
    ui->nameEdit->setText(QString::fromUtf8(event.getEventName().c_str()));
    ui->locationEdit->setText(QString::fromUtf8(event.getLocation().c_str()));
    ui->descriptionEdit->setPlainText(QString::fromUtf8(event.getDescription().c_str()));
    
    auto startTime = std::chrono::system_clock::to_time_t(event.getTimeSlot().getStartTime());
    auto endTime = std::chrono::system_clock::to_time_t(event.getTimeSlot().getEndTime());
    
    QDateTime startDateTime = QDateTime::fromSecsSinceEpoch(startTime);
    QDateTime endDateTime = QDateTime::fromSecsSinceEpoch(endTime);
    
    // 直接设置完整的日期时间
    ui->startTimeEdit->setDateTime(startDateTime);
    ui->endTimeEdit->setDateTime(endDateTime);
    ui->isCourseCheck->setChecked(event.getTimeSlot().getIsCourse());
}

void AddEventDialog::clear() {
    ui->nameEdit->clear();
    ui->locationEdit->clear();
    ui->descriptionEdit->clear();
    ui->startTimeEdit->setDateTime(QDateTime::currentDateTime());
    ui->endTimeEdit->setDateTime(QDateTime::currentDateTime().addSecs(3600));
    ui->isCourseCheck->setChecked(false);
}

void AddEventDialog::presetTimeRange(const QDateTime& start, const QDateTime& end) {
    QDateTime startTime = start.isValid() ? start : QDateTime::currentDateTime();
    QDateTime endTime = end.isValid() ? end : startTime.addSecs(3600);
    
    if (endTime <= startTime) {
        endTime = startTime.addSecs(3600);
    }
    
    ui->startTimeEdit->setDateTime(startTime);
    ui->endTimeEdit->setDateTime(endTime);
}
