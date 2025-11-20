#include "DataManager.h"
#include <fstream>
#include <sstream>
#include <iomanip>

DataManager::DataManager() {
}

bool DataManager::saveUserData(const User& userData, const std::string& filePath) {
    std::ofstream file(filePath);
    if (!file.is_open()) {
        return false;
    }

    // 保存用户名
    file << "USER:" << userData.getName() << "\n";
    
    // 保存课程
    file << "COURSES:\n";
    for (const auto& event : userData.getCourses().getAllEvents()) {
        auto startTime = std::chrono::system_clock::to_time_t(event.getTimeSlot().getStartTime());
        auto endTime = std::chrono::system_clock::to_time_t(event.getTimeSlot().getEndTime());
        
        file << event.getId() << ","
             << event.getEventName() << ","
             << event.getLocation() << ","
             << event.getDescription() << ","
             << event.getWeekday() << ","
             << startTime << ","
             << endTime << ","
             << event.getTimeSlot().getIsCourse() << ","
             << event.getTags() << "\n";
    }
    
    // 保存个人日程
    file << "PERSONAL:\n";
    for (const auto& event : userData.getPersonalSchedule().getAllEvents()) {
        auto startTime = std::chrono::system_clock::to_time_t(event.getTimeSlot().getStartTime());
        auto endTime = std::chrono::system_clock::to_time_t(event.getTimeSlot().getEndTime());
        
        file << event.getId() << ","
             << event.getEventName() << ","
             << event.getLocation() << ","
             << event.getDescription() << ","
             << event.getWeekday() << ","
             << startTime << ","
             << endTime << ","
             << event.getTimeSlot().getIsCourse() << ","
             << event.getTags() << "\n";
    }

    // 保存假期与屏蔽课程信息（同一输出流，避免重复追加）
    file << "HOLIDAYS:\n";
    for (const auto& h : holidays) {
        file << h.year << "-" << h.month << "-" << h.day << "," << h.name << "\n";
    }
    file << "SUPPRESS:\n";
    for (const auto& p : suppressedCourseWeeks) {
        file << p.first << "," << p.second << "\n";
    }
    file.close();
    return true;
}

bool DataManager::loadUserData(User& userData, const std::string& filePath) {
    std::ifstream file(filePath);
    if (!file.is_open()) {
        return false;
    }

    //加载前清空，避免重复累计
    userData.getCourses().clear();
    userData.getPersonalSchedule().clear();
    std::string line;
    std::string section;
    
    while (std::getline(file, line)) {
        if (line.empty()) continue;
        
        if (line.substr(0, 5) == "USER:") {
            userData.setName(line.substr(5));
        } else if (line == "COURSES:") {
            section = "COURSES";
        } else if (line == "PERSONAL:") {
            section = "PERSONAL";
        } else if (line == "HOLIDAYS:") {
            section = "HOLIDAYS";
        } else if (line == "SUPPRESS:") {
            section = "SUPPRESS";
        } else {
            // 解析事件
            std::istringstream iss(line);
            std::string id, name, location, description, weekday, startTime, endTime, isCourse, tags;
            
            std::getline(iss, id, ',');
            std::getline(iss, name, ',');
            std::getline(iss, location, ',');
            std::getline(iss, description, ',');
            std::getline(iss, weekday, ',');
            std::getline(iss, startTime, ',');
            std::getline(iss, endTime, ',');
            std::getline(iss, isCourse, ',');
            std::getline(iss, tags, ',');
            
            if (!id.empty() && (section == "COURSES" || section == "PERSONAL")) {
                std::time_t start_t = std::stoll(startTime);
                std::time_t end_t = std::stoll(endTime);
                
                TimeSlot slot(std::chrono::system_clock::from_time_t(start_t),
                            std::chrono::system_clock::from_time_t(end_t),
                            isCourse == "1");
                
                ScheduleEvent event(std::stoi(id), name, location, description,
                                  std::stoi(weekday), slot);
                
                // 设置标签
                if (!tags.empty()) {
                    event.setTags(std::stoi(tags));
                }
                
                if (section == "COURSES") {
                    userData.getCourses().addEvent(event);
                } else if (section == "PERSONAL") {
                    userData.getPersonalSchedule().addEvent(event);
                }
            } else if (section == "HOLIDAYS") {
                // 解析假期行：YYYY-MM-DD,Name
                std::string dateStr, holidayName;
                std::istringstream hss(line);
                std::getline(hss, dateStr, ',');
                std::getline(hss, holidayName, ',');
                if (!dateStr.empty()) {
                    int y=0,m=0,d=0;
                    char dash1='-', dash2='-';
                    std::istringstream ds(dateStr);
                    ds >> y >> dash1 >> m >> dash2 >> d;
                    if (y>0 && m>0 && d>0) {
                        holidays.push_back(HolidayItem{y,m,d,holidayName});
                    }
                }
            } else if (section == "SUPPRESS") {
                // 解析屏蔽课程行：eventId,weekOffset
                std::string eidStr, offStr;
                std::istringstream pss(line);
                std::getline(pss, eidStr, ',');
                std::getline(pss, offStr, ',');
                if (!eidStr.empty() && !offStr.empty()) {
                    suppressedCourseWeeks.emplace_back(std::stoi(eidStr), std::stoi(offStr));
                }
            }
        }
    }

    file.close();
    return true;
}

User& DataManager::getUser() {
    return user;
}

const User& DataManager::getUser() const {
    return user;
}

bool DataManager::loadProfessorsData(const std::string& filePath) {
    std::ifstream file(filePath);
    if (!file.is_open()) {
        return false;
    }

    professors.clear();
    std::string line;
    Professor* currentProf = nullptr;
    
    while (std::getline(file, line)) {
        if (line.empty()) continue;
        
        if (line.substr(0, 10) == "PROFESSOR:") {
            std::string profInfo = line.substr(10);
            std::istringstream iss(profInfo);
            std::string name, email;
            
            std::getline(iss, name, ',');
            std::getline(iss, email, ',');
            
            professors.push_back(Professor(name, email));
            currentProf = &professors.back();
        } else if (currentProf != nullptr) {
            // 解析办公时间
            std::istringstream iss(line);
            std::string id, name, location, description, weekday, startTime, endTime, isCourse, tags;
            
            std::getline(iss, id, ',');
            std::getline(iss, name, ',');
            std::getline(iss, location, ',');
            std::getline(iss, description, ',');
            std::getline(iss, weekday, ',');
            std::getline(iss, startTime, ',');
            std::getline(iss, endTime, ',');
            std::getline(iss, isCourse, ',');
            std::getline(iss, tags, ',');
            
            if (!id.empty()) {
                std::time_t start_t = std::stoll(startTime);
                std::time_t end_t = std::stoll(endTime);
                
                TimeSlot slot(std::chrono::system_clock::from_time_t(start_t),
                            std::chrono::system_clock::from_time_t(end_t),
                            isCourse == "1");
                
                ScheduleEvent event(std::stoi(id), name, location, description,
                                  std::stoi(weekday), slot);
                
                // 设置标签
                if (!tags.empty()) {
                    event.setTags(std::stoi(tags));
                }
                
                currentProf->getOfficeHours().addEvent(event);
            }
        }
    }

    file.close();
    return true;
}

const std::vector<Professor>& DataManager::getProfessors() const {
    return professors;
}

Professor DataManager::getProfessorByName(const std::string& name) const {
    for (const auto& prof : professors) {
        if (prof.getName() == name) {
            return prof;
        }
    }
    return Professor();
}

// 导入或合并教师数据：按姓名合并，存在替换，不存在追加
bool DataManager::importOrMergeProfessors(const std::vector<Professor>& profs) {
    bool changed = false;
    for (const auto& prof : profs) {
        bool found = false;
        for (auto& existing : professors) {
            if (existing.getName() == prof.getName()) {
                existing = prof; // 已存在则整体替换
                found = true;
                changed = true;
                break;
            }
        }
        if (!found) {
            professors.push_back(prof); // 不存在则追加
            changed = true;
        }
    }
    return changed;
}

bool DataManager::saveProfessorsData(const std::vector<Professor>& profs,
                                    const std::string& filePath) {
    std::ofstream file(filePath);
    if (!file.is_open()) {
        return false;
    }

    for (const auto& prof : profs) {
        file << "PROFESSOR:" << prof.getName() << "," << prof.getEmail() << "\n";
        
        for (const auto& event : prof.getOfficeHours().getAllEvents()) {
            auto startTime = std::chrono::system_clock::to_time_t(event.getTimeSlot().getStartTime());
            auto endTime = std::chrono::system_clock::to_time_t(event.getTimeSlot().getEndTime());
            
            file << event.getId() << ","
                 << event.getEventName() << ","
                 << event.getLocation() << ","
                 << event.getDescription() << ","
                 << event.getWeekday() << ","
                 << startTime << ","
                 << endTime << ","
                 << event.getTimeSlot().getIsCourse() << ","
                 << event.getTags() << "\n";
        }
    }

    file.close();
    return true;
}

void DataManager::setHolidays(const std::vector<HolidayItem>& items) {
    holidays = items;
}

const std::vector<DataManager::HolidayItem>& DataManager::getHolidays() const {
    return holidays;
}

void DataManager::setSuppressedCourseWeeks(const std::vector<std::pair<int,int>>& suppressed) {
    suppressedCourseWeeks = suppressed;
}

const std::vector<std::pair<int,int>>& DataManager::getSuppressedCourseWeeks() const {
    return suppressedCourseWeeks;
}

