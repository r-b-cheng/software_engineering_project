#include "Schedule.h"
#include <algorithm>
#include <ctime>

Schedule::Schedule() {
}

void Schedule::addEvent(const ScheduleEvent& event) {
    events.push_back(event);
}

bool Schedule::addEventSafely(const ScheduleEvent& event, std::string& errorMsg) {
    // 检查重复事件
    for (const auto& existingEvent : events) {
        // 检查名称、地点、星期和时间是否完全相同
        if (existingEvent.getEventName() == event.getEventName() && 
            existingEvent.getLocation() == event.getLocation() &&
            existingEvent.getTimeSlot().getStartTime() == event.getTimeSlot().getStartTime() &&
            existingEvent.getTimeSlot().getEndTime() == event.getTimeSlot().getEndTime()) {
            errorMsg = "事件重复";
            return false;
        }
        
        // 检查时间冲突
        if (existingEvent.getTimeSlot().isOverlappingWith(event.getTimeSlot())) {
            errorMsg = "时间冲突";
            return false;
        }
    }
    
    // 通过检查，添加事件
    events.push_back(event);
    return true;
}

bool Schedule::removeEvent(int eventId) {
    auto it = std::find_if(events.begin(), events.end(),
                          [eventId](const ScheduleEvent& e) {
                              return e.getId() == eventId;
                          });
    if (it != events.end()) {
        events.erase(it);
        return true;
    }
    return false;
}

std::vector<ScheduleEvent> Schedule::getEventsForDate(
    const std::chrono::system_clock::time_point& date) const {
    
    std::vector<ScheduleEvent> result;
    auto dateTime = std::chrono::system_clock::to_time_t(date);
    std::tm dateTm{};
    if (auto p = std::localtime(&dateTime)) {
        dateTm = *p;
    } else if (auto pg = std::gmtime(&dateTime)) {
        dateTm = *pg;
    }
    
    for (const auto& event : events) {
        auto eventTime = std::chrono::system_clock::to_time_t(event.getTimeSlot().getStartTime());
        std::tm eventTm{};
        if (auto p2 = std::localtime(&eventTime)) {
            eventTm = *p2;
        } else if (auto p2g = std::gmtime(&eventTime)) {
            eventTm = *p2g;
        }
        
        if (dateTm.tm_year == eventTm.tm_year &&
            dateTm.tm_mon == eventTm.tm_mon &&
            dateTm.tm_mday == eventTm.tm_mday) {
            result.push_back(event);
        }
    }
    
    return result;
}

// 辅助：将 time_point 的秒数归零到分钟粒度
static std::chrono::system_clock::time_point roundSecondsToZero(const std::chrono::system_clock::time_point& tp) {
    std::time_t tt = std::chrono::system_clock::to_time_t(tp);
    std::time_t floored = (tt / 60) * 60;
    return std::chrono::system_clock::from_time_t(floored);
}

// 辅助：获取目标周一 00:00 的 time_point（相对当前周的偏移）
static std::chrono::system_clock::time_point getMondayMidnight(int weekOffset) {
    auto now = std::chrono::system_clock::now();
    std::time_t nowTt = std::chrono::system_clock::to_time_t(now);
    std::tm nowTm = *std::localtime(&nowTt);
    nowTm.tm_hour = 0;
    nowTm.tm_min = 0;
    nowTm.tm_sec = 0;
    std::time_t midnightTodayTt = std::mktime(&nowTm);
    int daysToMonday = (nowTm.tm_wday + 6) % 7; // 将星期换算为距离周一的天数
    std::time_t mondayTt = midnightTodayTt - static_cast<std::time_t>(daysToMonday * 24 * 60 * 60);
    mondayTt += static_cast<std::time_t>(weekOffset * 7 * 24 * 60 * 60);
    return std::chrono::system_clock::from_time_t(mondayTt);
}

// 辅助：把事件按 weekday 和原时分归一化到目标周（健壮的时间解析）
static TimeSlot normalizeEventToWeek(const ScheduleEvent& event,
                                     int weekOffset,
                                     const std::chrono::system_clock::time_point& mondayMidnight) {
    int daysIntoWeek = event.getWeekday() - 1; // MONDAY=1..SUNDAY=7
    auto dayMidnight = mondayMidnight + std::chrono::hours(24 * daysIntoWeek);

    std::time_t startTt = std::chrono::system_clock::to_time_t(event.getTimeSlot().getStartTime());
    std::tm startTm{};
    if (auto ptr = std::localtime(&startTt)) {
        startTm = *ptr;
    } else if (auto gptr = std::gmtime(&startTt)) {
        startTm = *gptr;
    }

    auto normalizedStart = dayMidnight + std::chrono::hours(startTm.tm_hour) + std::chrono::minutes(startTm.tm_min);

    // 关键修复：使用原事件的时长来计算归一化后的结束时间，避免跨午夜等场景出错
    auto originalDuration = event.getTimeSlot().getEndTime() - event.getTimeSlot().getStartTime();
    auto durationMinutes = std::chrono::duration_cast<std::chrono::minutes>(originalDuration);

    auto start = roundSecondsToZero(normalizedStart);
    auto end   = start + durationMinutes;
    return TimeSlot(start, end, event.getTimeSlot().getIsCourse());
}

const std::vector<ScheduleEvent>& Schedule::getEventsForWeek(int weekOffset) const {
    // 简化实现：直接返回所有事件
    // 实际应用中应该根据weekOffset计算对应周的起止时间
    return events;
}

std::vector<ScheduleEvent> Schedule::getEventsInRange(
    const std::chrono::system_clock::time_point& start,
    const std::chrono::system_clock::time_point& end) const {
    
    std::vector<ScheduleEvent> result;
    
    for (const auto& event : events) {
        auto eventStart = event.getTimeSlot().getStartTime();
        auto eventEnd = event.getTimeSlot().getEndTime();
        
        // 检查事件是否在时间范围内
        if (!(eventEnd <= start || eventStart >= end)) {
            result.push_back(event);
        }
    }
    
    return result;
}

Schedule Schedule::operator+(const Schedule& another) const {
    Schedule result;
    result.events = this->events;
    result.events.insert(result.events.end(),
                        another.events.begin(),
                        another.events.end());
    return result;
}

const std::vector<ScheduleEvent>& Schedule::getAllEvents() const {
    return events;
}

void Schedule::clear() {
    events.clear();
}

std::vector<ScheduleEvent> Schedule::getEventsForWeekCopy(int weekOffset) const {
    std::vector<ScheduleEvent> result;
    auto mondayMidnight = getMondayMidnight(weekOffset);

    for (const auto& event : events) {
        if (event.getTimeSlot().getIsCourse()) {
            // 课程事件：按目标周归一化（每周重复），实现“课程全加”
            TimeSlot normSlot = normalizeEventToWeek(event, weekOffset, mondayMidnight);
            ScheduleEvent copy = event;
            copy.setTimeSlot(normSlot);
            result.push_back(copy);
        } else {
            // 非课程（个人日程）：仅保留该周的事件，时间不变，体现“日程正常加”
            if (event.getWeekOffset() == weekOffset) {
                result.push_back(event);
            }
        }
    }

    return result;
}


