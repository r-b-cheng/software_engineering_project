#include "SchedulerLogic.h"
#include <algorithm>
#include <ctime>

// 辅助函数：将 time_point 的秒数归零
static std::chrono::system_clock::time_point roundSecondsToZero(const std::chrono::system_clock::time_point& tp) {
    // 使用 time_t 进行按分钟向下取整，避免使用不安全的 localtime/mktime
    std::time_t tt = std::chrono::system_clock::to_time_t(tp);
    std::time_t floored = (tt / 60) * 60; // 去除秒部分
    return std::chrono::system_clock::from_time_t(floored);
}
//按周偏移进行可用时间计算
std::vector<TimeSlot> SchedulerLogic::findAvailableSlots(
    const Schedule& studentSchedule,
    const Schedule& officeHour,
    int weekOffset) {

    std::vector<TimeSlot> availableSlots;

    // 获得当前周的日程
    const auto studentEvents = studentSchedule.getEventsForWeekCopy(weekOffset);
    // 对于老师的office time 全部归一化到目标周
    const auto officeEvents  = officeHour.getEventsForWeekCopy(weekOffset);


    for (const auto& officeEvent : officeEvents) {
        std::vector<TimeSlot> slots = { officeEvent.getTimeSlot() };
        for (const auto& studentEvent : studentEvents) {
            std::vector<TimeSlot> newSlots;
            for (const auto& slot : slots) {
                const TimeSlot& studentSlot = studentEvent.getTimeSlot();
                if (slot.isOverlappingWith(studentSlot)) {
                    if (studentSlot.getStartTime() > slot.getStartTime()) {
                        auto start = roundSecondsToZero(slot.getStartTime());
                        auto end   = roundSecondsToZero(studentSlot.getStartTime());
                        newSlots.push_back(TimeSlot(start, end, false));
                    }
                    if (studentSlot.getEndTime() < slot.getEndTime()) {
                        auto newStart = roundSecondsToZero(studentSlot.getEndTime());
                        auto newEnd   = roundSecondsToZero(slot.getEndTime());
                        if (newStart < newEnd) {
                            newSlots.push_back(TimeSlot(newStart, newEnd, false));
                        }
                    }
                } else {
                    auto start = roundSecondsToZero(slot.getStartTime());
                    auto end   = roundSecondsToZero(slot.getEndTime());
                    newSlots.push_back(TimeSlot(start, end, false));
                }
            }
            slots = newSlots;
            if (slots.empty()) break;
        }
        for (const auto& slot : slots) {
            if (slot.durationMinutes() > 30) {  // 忽略时长小于30分钟的空闲时间
                availableSlots.push_back(slot);
            }
        }
    }
    return availableSlots;
}

