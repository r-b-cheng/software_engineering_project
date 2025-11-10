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
                // 不做 roundToHour，直接加入真实空闲段
                availableSlots.push_back(slot);
            }
        }
    }
    return availableSlots;
}

