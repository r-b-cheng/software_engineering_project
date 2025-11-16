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

public:
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
};

#endif // DATAMANAGER_H

