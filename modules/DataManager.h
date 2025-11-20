#ifndef DATAMANAGER_H
#define DATAMANAGER_H

#include "../datastructure/User.h"
#include "../datastructure/Professor.h"
#include <vector>
#include <string>

class DataManager {
private:
    User user;
    std::vector<Professor> professors;
    std::vector<std::pair<int,int>> suppressedCourseWeeks; // (eventId, weekOffset)

public:
    struct HolidayItem { int year; int month; int day; std::string name; };
    std::vector<HolidayItem> holidays;
    DataManager();

    // 保存学生数据到文件
    bool saveUserData(const User& userData, const std::string& filePath);
    
    // 从文件加载学生数据
    bool loadUserData(User& userData, const std::string& filePath);
    
    // 获取用户对象
    User& getUser();
    const User& getUser() const;
    
    // 从文件加载教师数据
    bool loadProfessorsData(const std::string& filePath);
    
    // 获取教师列表
    const std::vector<Professor>& getProfessors() const;
    
    // 根据姓名获取教师信息
    Professor getProfessorByName(const std::string& name) const;
    
    // 导入或合并教师数据
    bool importOrMergeProfessors(const std::vector<Professor>& profs);
    
    // 保存教师信息
    bool saveProfessorsData(const std::vector<Professor>& profs, const std::string& filePath);

    // 假期与屏蔽课程的持久化数据访问
    void setHolidays(const std::vector<HolidayItem>& items);
    const std::vector<HolidayItem>& getHolidays() const;
    void setSuppressedCourseWeeks(const std::vector<std::pair<int,int>>& suppressed);
    const std::vector<std::pair<int,int>>& getSuppressedCourseWeeks() const;
};

#endif // DATAMANAGER_H

