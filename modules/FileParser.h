#ifndef FILEPARSER_H
#define FILEPARSER_H

#include "../datastructure/Schedule.h"
#include "../datastructure/Professor.h"
#include <string>
#include <vector>

class FileParser {
public:
    // 解析CSV文件，生成Schedule
    static Schedule parseCsv(const std::string& filePath);
    
    // 解析教师CSV文件
    static std::vector<Professor> parseProfessorsCsv(const std::string& filePath);

    // 解析ICS文本，提取公共假期事件（DTSTART/DTEND/SUMMARY）
    static std::vector<ScheduleEvent> parseIcsHolidays(const std::string& icsContent);
};

#endif // FILEPARSER_H

