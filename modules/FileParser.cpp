#include "FileParser.h"
#include <fstream>
#include <sstream>
#include <ctime>
#include <iomanip>
#include <algorithm>
#include <regex>

// 静态辅助函数：去除首尾空格、CR、引号
static void trim(std::string& s) {
    auto not_space = [](unsigned char ch){ return !std::isspace(ch); };
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), not_space));
    s.erase(std::find_if(s.rbegin(), s.rend(), not_space).base(), s.end());
    // 去除两端引号（如果存在）
    if (s.size() >= 2 && s.front() == '"' && s.back() == '"') {
        s = s.substr(1, s.size() - 2);
    }
}

Schedule FileParser::parseCsv(const std::string& filePath) {
    Schedule schedule;
    std::ifstream file(filePath);

    if (!file.is_open()) {
        return schedule;
    }

    std::string line;
    // 跳过表头
    std::getline(file, line);

    int eventId = 1;
    while (std::getline(file, line)) {
        trim(line);
        if (line.empty()) continue;

        std::istringstream iss(line);
        std::string name, location, description, weekdayStr, startTimeStr, endTimeStr, isCourseStr;

        std::getline(iss, name, ',');
        std::getline(iss, location, ',');
        std::getline(iss, description, ',');
        std::getline(iss, weekdayStr, ',');
        std::getline(iss, startTimeStr, ',');
        std::getline(iss, endTimeStr, ',');
        std::getline(iss, isCourseStr, ',');

        trim(name); trim(location); trim(description); trim(weekdayStr);
        trim(startTimeStr); trim(endTimeStr); trim(isCourseStr);

        if (!name.empty()) {
            // 解析时间（假设格式为 YYYY-MM-DD HH:MM）
            std::tm start_tm = {};
            std::tm end_tm = {};

            std::istringstream start_ss(startTimeStr);
            start_ss >> std::get_time(&start_tm, "%Y-%m-%d %H:%M");

            std::istringstream end_ss(endTimeStr);
            end_ss >> std::get_time(&end_tm, "%Y-%m-%d %H:%M");

            std::time_t start_t = std::mktime(&start_tm);
            std::time_t end_t = std::mktime(&end_tm);

            bool isCourse = (isCourseStr == "1" || isCourseStr == "true");

            TimeSlot slot(std::chrono::system_clock::from_time_t(start_t),
                          std::chrono::system_clock::from_time_t(end_t),
                          isCourse);

            int weekday = std::stoi(weekdayStr);
            ScheduleEvent event(eventId++, name, location, description, weekday, slot);

            schedule.addEvent(event);
        }
    }

    file.close();
    return schedule;
}

std::vector<Professor> FileParser::parseProfessorsCsv(const std::string& filePath) {
    std::vector<Professor> professors;
    std::ifstream file(filePath);

    if (!file.is_open()) {
        return professors;
    }

    std::string line;
    // 跳过表头
    std::getline(file, line);

    std::string currentProfName;
    std::string currentProfEmail;
    Professor* currentProf = nullptr;
    int eventId = 1;

    while (std::getline(file, line)) {
        trim(line);
        if (line.empty()) continue;

        std::istringstream iss(line);
        std::string profName, profEmail, eventName, location, description, weekdayStr, startTimeStr, endTimeStr;

        std::getline(iss, profName, ',');
        std::getline(iss, profEmail, ',');
        std::getline(iss, eventName, ',');
        std::getline(iss, location, ',');
        std::getline(iss, description, ',');
        std::getline(iss, weekdayStr, ',');
        std::getline(iss, startTimeStr, ',');
        std::getline(iss, endTimeStr, ',');

        trim(profName); trim(profEmail); trim(eventName); trim(location);
        trim(description); trim(weekdayStr); trim(startTimeStr); trim(endTimeStr);

        // 如果是新教师
        if (!profName.empty() && (profName != currentProfName)) {
            currentProfName = profName;
            currentProfEmail = profEmail;
            professors.push_back(Professor(profName, profEmail));
            currentProf = &professors.back();
        }

        // 添加办公时间
        if (currentProf != nullptr && !eventName.empty()) {
            std::time_t start_t;
            std::time_t end_t;

            //这里是定义了一个lambda表达式当做临时的函数使用，用来解析时间字符串。
            auto parseTime = [](const std::string& s, std::time_t& out) -> bool {
                std::tm tm = {};
                std::istringstream ss(s);
                ss >> std::get_time(&tm, "%Y-%m-%d %H:%M");
                if (!ss.fail()) {
                    out = std::mktime(&tm);
                    return true;
                }
                try {
                    out = static_cast<std::time_t>(std::stoll(s));
                    return true;
                } catch (...) {
                    return false;
                }
            };

            //如果解析失败或者开始时间比结束时间大，则跳过不插入
            if (!parseTime(startTimeStr, start_t) || !parseTime(endTimeStr, end_t)) {
                continue;
            }
            if (end_t <= start_t) {
                continue;
            }

            int weekday;
            try {
                weekday = std::stoi(weekdayStr);
            } catch (...) {
                continue;
            }

            TimeSlot slot(std::chrono::system_clock::from_time_t(start_t),
                          std::chrono::system_clock::from_time_t(end_t),
                          true);  // 办公时间标记为true

            ScheduleEvent event(eventId++, eventName, location, description, weekday, slot);
            currentProf->getOfficeHours().addEvent(event);
        }
    }

    file.close();
    return professors;
}

std::vector<ScheduleEvent> FileParser::parseIcsHolidays(const std::string& icsContent) {
    std::vector<ScheduleEvent> events;
    std::istringstream ss(icsContent);
    std::string line;
    bool inEvent = false;
    std::string dtstart;
    std::string dtend;
    std::string summary;

    auto parseIcsDateTime = [](const std::string& s, std::time_t& out) -> bool {
        // Handles formats like YYYYMMDD or YYYYMMDDTHHMMSSZ
        std::tm tm = {};
        try {
            if (s.size() >= 8) {
                tm.tm_year = std::stoi(s.substr(0,4)) - 1900;
                tm.tm_mon  = std::stoi(s.substr(4,2)) - 1;
                tm.tm_mday = std::stoi(s.substr(6,2));
                if (s.size() >= 15 && s[8] == 'T') {
                    tm.tm_hour = std::stoi(s.substr(9,2));
                    tm.tm_min  = std::stoi(s.substr(11,2));
                    tm.tm_sec  = std::stoi(s.substr(13,2));
                } else {
                    tm.tm_hour = 0; tm.tm_min = 0; tm.tm_sec = 0;
                }
                // Treat as local time
                out = std::mktime(&tm);
                return out != -1;
            }
        } catch (...) {}
        return false;
    };

    while (std::getline(ss, line)) {
        trim(line);
        if (line.rfind("BEGIN:VEVENT", 0) == 0) {
            inEvent = true; dtstart.clear(); dtend.clear(); summary.clear();
        } else if (line.rfind("END:VEVENT", 0) == 0) {
            if (inEvent && !summary.empty() && !dtstart.empty()) {
                std::time_t start_t = 0, end_t = 0;
                if (!parseIcsDateTime(dtstart, start_t)) { inEvent = false; continue; }
                if (!dtend.empty()) parseIcsDateTime(dtend, end_t);
                if (end_t == 0 || end_t <= start_t) end_t = start_t + 3600; // default 1h

                TimeSlot slot(std::chrono::system_clock::from_time_t(start_t),
                              std::chrono::system_clock::from_time_t(end_t),
                              false);
                // weekday: 1..7 (Qt style), but we store int
                std::tm* stm = std::localtime(&start_t);
                int weekday = stm ? ((stm->tm_wday == 0) ? 7 : stm->tm_wday) : 1;
                ScheduleEvent ev(0, summary, "", "公共假期", weekday, slot);
                events.push_back(ev);
            }
            inEvent = false;
        } else if (inEvent) {
            if (line.rfind("DTSTART", 0) == 0) {
                auto pos = line.find(":");
                if (pos != std::string::npos) dtstart = line.substr(pos+1);
            } else if (line.rfind("DTEND", 0) == 0) {
                auto pos = line.find(":");
                if (pos != std::string::npos) dtend = line.substr(pos+1);
            } else if (line.rfind("SUMMARY", 0) == 0) {
                auto pos = line.find(":");
                if (pos != std::string::npos) summary = line.substr(pos+1);
            }
        }
    }

    return events;
}

