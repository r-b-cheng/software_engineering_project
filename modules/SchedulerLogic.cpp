#include "SchedulerLogic.h"
#include <algorithm>

// 计算学生和教师办公时间的可用空闲时间段
std::vector<TimeSlot> SchedulerLogic::findAvailableSlots(
    const Schedule& studentSchedule,
    const Schedule& officeHour) {

    std::vector<TimeSlot> availableSlots;
    // 获取教师办公时间和学生个人日程的所有事件
    const auto& officeEvents = officeHour.getAllEvents();
    const auto& studentEvents = studentSchedule.getAllEvents();
    
    // 遍历每个教师办公时间事件
    for (const auto& officeEvent : officeEvents) {
        // 初始可用段为当前办公时间段
        std::vector<TimeSlot> slots = { officeEvent.getTimeSlot() };
        // 依次排除学生日程中的冲突时间段
        for (const auto& studentEvent : studentEvents) {
            std::vector<TimeSlot> newSlots;
            for (const auto& slot : slots) {
                TimeSlot studentSlot = studentEvent.getTimeSlot();
                // 判断是否有重叠
                if (slot.isOverlappingWith(studentSlot)) {
                    // 学生事件开始前的空闲段
                    if (studentSlot.getStartTime() > slot.getStartTime()) {
                        newSlots.push_back(TimeSlot(slot.getStartTime(), studentSlot.getStartTime(), false));
                    }
                    // 学生事件结束后的空闲段
                    if (studentSlot.getEndTime() < slot.getEndTime()) {
                        auto newStart = studentSlot.getEndTime();
                        if (newStart < slot.getEndTime()) {
                            newSlots.push_back(TimeSlot(newStart, slot.getEndTime(), false));
                        }
                    }
                } else {
                    // 无重叠则保留原空闲段
                    newSlots.push_back(slot);
                }
            }
            slots = newSlots;
            // 若已无可用空闲段则提前结束
            if (slots.empty()) break;
        }
        // 仅将时长大于0的空闲段加入结果
        for (const auto& slot : slots) {
            if (slot.durationMinutes() > 0) {
                auto roundToHour = [](std::chrono::system_clock::time_point tp) {
                    std::time_t t = std::chrono::system_clock::to_time_t(tp);
                    struct tm* timeinfo = std::localtime(&t);
                    timeinfo->tm_min = 0;
                    timeinfo->tm_sec = 0;
                    return std::chrono::system_clock::from_time_t(std::mktime(timeinfo));
                };
                auto slotStart = roundToHour(slot.getStartTime());
                auto slotEnd = roundToHour(slot.getEndTime());
                if (std::chrono::duration_cast<std::chrono::minutes>(slotEnd - slotStart).count() > 0) {
                    availableSlots.emplace_back(slotStart, slotEnd);
                }
            }
        }
    }
    return availableSlots;
}

