#ifndef SCHEDULE_H
#define SCHEDULE_H

#include "ScheduleEvent.h"
#include <vector>
#include <chrono>
#include <string>

class Schedule {
private:
    std::vector<ScheduleEvent> events;

public:
    Schedule();

    // 添加新事件
    void addEvent(const ScheduleEvent& event);
    
    // 安全添加事件（检查冲突和重复）
    bool addEventSafely(const ScheduleEvent& event, std::string& errorMsg);
    
    // 根据事件编号删除事件
    bool removeEvent(int eventId);
    
    // 获取某一天的事件列表
    std::vector<ScheduleEvent> getEventsForDate(const std::chrono::system_clock::time_point& date) const;
    
    // 获取一周的所有事件
    const std::vector<ScheduleEvent>& getEventsForWeek(int weekOffset) const;
    
    // 获取时间范围内的事件
    std::vector<ScheduleEvent> getEventsInRange(
        const std::chrono::system_clock::time_point& start,
        const std::chrono::system_clock::time_point& end) const;
    
    // 重载+运算符，合并两个日程
    Schedule operator+(const Schedule& another) const;
    
    // 获取所有事件
    const std::vector<ScheduleEvent>& getAllEvents() const;
    
    // 清空所有事件
    void clear();

    //返回指定周的事件副本（课程按周归一化，个人日程仅该周）  
    //因为老师的office hour在导入时iscourse都为true 所以会直接将所有的officetime都归一化到这一周
    std::vector<ScheduleEvent> getEventsForWeekCopy(int weekOffset) const;

};

#endif // SCHEDULE_H

