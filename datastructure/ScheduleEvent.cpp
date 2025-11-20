#include "ScheduleEvent.h"
#include <ctime>
#include <chrono>
#include <QDate>



ScheduleEvent::ScheduleEvent()
    : id(0), weekday(MONDAY), tags(0) {
}

ScheduleEvent::ScheduleEvent(int eventId, const std::string& name,
                             const std::string& loc, const std::string& desc,
                             int day, const TimeSlot& slot, int eventTags)
    : id(eventId), eventName(name), location(loc),
      description(desc), weekday(day), timeSlot(slot), tags(eventTags) {
}

int ScheduleEvent::getId() const {
    return id;
}

std::string ScheduleEvent::getEventName() const {
    return eventName;
}

std::string ScheduleEvent::getLocation() const {
    return location;
}

std::string ScheduleEvent::getDescription() const {
    return description;
}

int ScheduleEvent::getWeekday() const {
    return weekday;
}

TimeSlot ScheduleEvent::getTimeSlot() const {
    return timeSlot;
}

int ScheduleEvent::getTags() const {
    return tags;
}

// 标签相关方法
bool ScheduleEvent::hasTag(EventTag tag) const {
    return (tags & tag) != 0;
}

void ScheduleEvent::addTag(EventTag tag) {
    tags |= tag;
}

void ScheduleEvent::removeTag(EventTag tag) {
    tags &= ~tag;
}

void ScheduleEvent::setTags(int newTags) {
    tags = newTags;
}

std::vector<std::string> ScheduleEvent::getTagNames() const {
    std::vector<std::string> tagNames;
    
    if (hasTag(TAG_MIDTERM)) tagNames.push_back("期中");
    if (hasTag(TAG_FINAL)) tagNames.push_back("期末");
    if (hasTag(TAG_REVIEW)) tagNames.push_back("复习");
    if (hasTag(TAG_MAKEUP)) tagNames.push_back("补课");
    if (hasTag(TAG_PRE)) tagNames.push_back("Pre");
    if (hasTag(TAG_URGENT)) tagNames.push_back("紧急");
    if (hasTag(TAG_IMPORTANT)) tagNames.push_back("重要");
    
    return tagNames;
}

void ScheduleEvent::setId(int eventId) {
    id = eventId;
}

void ScheduleEvent::setEventName(const std::string& name) {
    eventName = name;
}

void ScheduleEvent::setLocation(const std::string& loc) {
    location = loc;
}

void ScheduleEvent::setDescription(const std::string& desc) {
    description = desc;
}

void ScheduleEvent::setWeekday(int day) {
    weekday = day;
}

void ScheduleEvent::setTimeSlot(const TimeSlot& slot) {
    timeSlot = slot;
}

// 辅助函数实现
int ScheduleEvent::getWeekOffset() const {
    // 获取事件的时间点
    auto eventTime = timeSlot.getStartTime();
    std::time_t eventTt = std::chrono::system_clock::to_time_t(eventTime);
    // 安全地复制 localtime 结果，避免静态缓冲区被覆盖
    std::tm eventTm{};
    if (auto p = std::localtime(&eventTt)) {
        eventTm = *p;
    } else if (auto pg = std::gmtime(&eventTt)) { // 罕见情况下的回退
        eventTm = *pg;
    }
    
    // 转换为日期对象以便计算
    QDate eventDate(eventTm.tm_year + 1900, eventTm.tm_mon + 1, eventTm.tm_mday);
    
    // 计算事件日期所在周的周一
    int eventDaysToMonday = eventDate.dayOfWeek() - 1;
    QDate eventWeekMonday = eventDate.addDays(-eventDaysToMonday);
    
    // 获取当前时间
    auto now = std::chrono::system_clock::now();
    std::time_t nowTt = std::chrono::system_clock::to_time_t(now);
    std::tm nowTm{};
    if (auto p2 = std::localtime(&nowTt)) {
        nowTm = *p2;
    } else if (auto p2g = std::gmtime(&nowTt)) { // 罕见情况下的回退
        nowTm = *p2g;
    }
    QDate nowDate(nowTm.tm_year + 1900, nowTm.tm_mon + 1, nowTm.tm_mday);
    
    // 计算当前日期所在周的周一
    int daysToMonday = nowDate.dayOfWeek() - 1;  // Qt中周一=1
    QDate currentWeekMonday = nowDate.addDays(-daysToMonday);
    
    // 计算两个周一之间的天数差，然后除以7得到周差
    return currentWeekMonday.daysTo(eventWeekMonday) / 7; // 正值表示未来，负值表示过去
}


