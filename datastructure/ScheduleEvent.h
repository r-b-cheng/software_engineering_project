#ifndef SCHEDULEEVENT_H
#define SCHEDULEEVENT_H

#include "TimeSlot.h"
#include <string>
#include <vector>

enum Weekday {
    MONDAY = 1,    // 周一
    TUESDAY = 2,   // 周二
    WEDNESDAY = 3, // 周三
    THURSDAY = 4,  // 周四
    FRIDAY = 5,    // 周五
    SATURDAY = 6,  // 周六
    SUNDAY = 7     // 周日
};

enum EventTag {
    TAG_NONE = 0,
    TAG_MIDTERM = 1,    // 期中
    TAG_FINAL = 2,      // 期末
    TAG_REVIEW = 4,     // 复习
    TAG_MAKEUP = 8,     // 补课
    TAG_PRE = 16,       // pre
    TAG_URGENT = 32,    // 紧急
    TAG_IMPORTANT = 64  // 重要
};

class ScheduleEvent {
private:
    int id;
    std::string eventName;
    std::string location;
    std::string description;
    int weekday;
    TimeSlot timeSlot;
    int tags; // 使用位标志存储多个标签

public:
    ScheduleEvent();
    ScheduleEvent(int eventId, const std::string& name, const std::string& loc,
                  const std::string& desc, int day, const TimeSlot& slot, int eventTags = 0);

    // Getters
    int getId() const;
    std::string getEventName() const;
    std::string getLocation() const;
    std::string getDescription() const;
    int getWeekday() const;
    TimeSlot getTimeSlot() const;
    int getTags() const;
    
    // 标签相关方法
    bool hasTag(EventTag tag) const;
    void addTag(EventTag tag);
    void removeTag(EventTag tag);
    void setTags(int tags);
    std::vector<std::string> getTagNames() const;

    // Setters
    void setId(int eventId);
    void setEventName(const std::string& name);
    void setLocation(const std::string& loc);
    void setDescription(const std::string& desc);
    void setWeekday(int day);
    void setTimeSlot(const TimeSlot& slot);
    
    // 辅助函数
    int getWeekOffset() const;  // 当月周序，从0开始
};

#endif // SCHEDULEEVENT_H