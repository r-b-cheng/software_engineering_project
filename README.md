# Student Schedule Management System (Schedule Manager)

## Project Introduction
The “Muster’s Calendar” is a desktop application developed based on Qt5/C++ and designed specifically for student groups. 

It aims to help users efficiently manage their personal schedules and course arrangements, and can intelligently calculate the available meeting times with teachers, thereby improving time management efficiency.


## Functional Features

1. **Schedule Management**
   - Supports adding, editing, and deleting course schedules and personal schedules
   
   - Supports adding tags to events (such as midterm, final, review, etc.) for easy classification and management
   - Weekly view display mode, allowing free switching between different weeks for viewing
   
   - Weekly view display mode, allowing free switching between different weeks for viewing
   

2. **Teacher Office Hours Management**
   - Batch import teacher information and office hours from CSV files
   
   - Batch import teacher information and office hours from CSV files
   
   - Automatically merge duplicate teacher information (identified by name)

3. **Course and Schedule Import**
   - Support importing student course information from CSV files
   
   - Distinguish between courses and personal schedules, and manage them separately
   

4. **Intelligent time calculation**
   - Automatically analyze time conflicts between students and designated teachers

   - Exclude students' existing courses and personal schedules, and filter available meeting times

   - Take holiday factors into account and avoid scheduling meetings during holidays

5. **Data persistence**
   - Automatically save user data to local files

   - Historical data is automatically loaded when the program starts
   - Data storage path
      Under the data_storage/ directory
       user_data.txt: User courses and personal schedules
       professor_data.txt: Teacher information and office hours
   

## Project Structure

```
project/
├── datastructure/         # data structure definitions
│   ├── TimeSlot.h/cpp
│   ├── ScheduleEvent.h/cpp
│   ├── Schedule.h/cpp
│   ├── Professor.h/cpp
│   └── User.h/cpp
├── modules/              # business logic modules
│   ├── DataManager.h/cpp
│   ├── FileParser.h/cpp
│   └── SchedulerLogic.h/cpp
├── ui/                   # Qt user interface components
│   ├── MainWindow.h/cpp
│   ├── ScheduleView.h/cpp
│   ├── AddEventDialog.h/cpp
│   ├── ImportProfessorDialog.h/cpp
│   └── ResultDisplayWidget.h/cpp
├── example_data/         # example data files
│   ├── professors.csv
│   └── student_schedule.csv
├── data_storage/         # automatically generated data storage directory
├── main.cpp
├── ScheduleManager.pro
└── README.md
```

## Compile and Run

### Prerequisites

- Qt 5.x or a higher version
- A C++11 or higher version compiler
- qmake build tool

### Installation and Operation

#### Qt Creator（recommended）

1. Open Qt Creator
[![Open Qt Creator](installation\step1-QtWindow.jpg)]

2. Drag the file `ScheduleManager.pro` into the Qt Creator window
[![Drag .pro into Qt Creator](installation\step2-OpenFile.png)]

3. Click the “run” button
Click the "Run" button
[![Click Run]("installation\step3-Run.png)]

4. The management system project will pop up as a window.
The management system window will pop up
[![Program Window](installation\step4-PoppedWinodw.png)]

#### cmd

```bash
# Windows
qmake ScheduleManager.pro
nmake         # or mingw32-make (if using MinGW)

# Linux/macOS
qmake ScheduleManager.pro
make

# run the program
./bin/ScheduleManager  # Linux/macOS
bin\ScheduleManager.exe  # Windows
```

## usage

### 1. Add Events

1. Click the "Add Schedule" button on the toolbar
2. Fill in event details:
   - Event name
   - Location
   - Notes
   - Day of week
   - Start and end time
   - If it is a course (check "Course/Office Hours")
3. Click "OK" to save
[![image](usage\add_event.png)]

### 2. Delete Events

1. Click the icon with the left mouse button
2. Click "Delete the Event"
[![image](usage\delete_event.png)]


### 3. Add Teacher Schedule

1. Prepare a CSV-format file of teacher data (refer to `example_data/professors.csv`)
2. Click the "Import Teacher Time" button on the toolbar
3. Select the CSV file
4. Click "Import"
[![image](usage\add_teacherschedule.png)]


### 4. Add Student Schedule

1. Prepare a CSV-format student course file (refer to `example_data/student_schedule.csv`)
2. Click the "Import Student Courses" button on the toolbar
3. Select the CSV file
4. Click "Import"
[![image](usage\add_studentschedule.png)]

### 5. Calculate Available Time Slots

1. Click the "Calculate Available Time" button on the toolbar
2. Select a teacher from the drop-down list
3. The system will automatically calculate and display all available meeting time slots
4. The result window will show the teacher's contact information and a list of available times
[![image](usage\calculate_timeslots.png)]

### 5. Detect events 

1. The main interface displays all schedules in a weekly view
2. Courses are shown with a light blue background.
3. Personal schedules are shown with a light yellow background
4. Use the "Previous Week" / "Next Week" buttons to switch the weekly view
[![image](usage\detect_event.png)]

### 6. Modify the tag of the events
1. Double-click an event to modify detailed information
2. In the dialog box, you can modify the event name, location, description, day of week, start time, end time, and whether it is a course
3. Click "OK" to save the modifications
[![image](usage\view_and_modify.png)]



## CSV File format

### Teacher's office hours (professors.csv)

```csv
ProfessorName,Email,EventName,Location,Description,Weekday,StartTime,EndTime
Dr. Zhang,zhang@university.edu,Office Hour,Room 301,Weekly office hour,1,2025-01-06 14:00,2025-01-06 16:00
```

Field Description：
- ProfessorName: 教师姓名
- Email: 教师邮箱
- EventName: 办公时间名称
- Location: 地点
- Description: 描述
- Weekday: 星期几（1=周一, 7=周日）
- StartTime: 开始时间（格式：YYYY-MM-DD HH:MM）
- EndTime: 结束时间（格式：YYYY-MM-DD HH:MM）

### Student schedule (student_schedule.csv)

```csv
EventName,Location,Description,Weekday,StartTime,EndTime,IsCourse
Math Class,Building A Room 101,Calculus lecture,1,2025-01-06 10:00,2025-01-06 12:00,1
```

字段说明：
- EventName: 事件名称
- Location: 地点
- Description: 描述
- Weekday: 星期几（1=周一, 7=周日）
- StartTime: 开始时间（格式：YYYY-MM-DD HH:MM）
- EndTime: 结束时间（格式：YYYY-MM-DD HH:MM）
- IsCourse: 是否为课程（1=是, 0=否）


## Explanation of Core Classes

### data structure

- **TimeSlot**: 表示时间段，包含开始时间、结束时间和是否为课程的标记
- **ScheduleEvent**: 日程事件，包含事件名称、地点、描述等信息
- **Schedule**: 日程集合，管理多个日程事件
- **Professor**: 教师信息，包含姓名、邮箱和办公时间
- **User**: 学生用户，包含课程和个人日程

### Business logic module

- **DataManager**: 数据管理器，负责加载和保存数据
- **FileParser**: 文件解析器，解析CSV格式的数据文件
- **SchedulerLogic**: 调度逻辑，计算学生和教师之间的可用时间

### UI components

- **MainWindow**: 主窗口，程序的主界面
- **ScheduleView**: 日程视图，以周视图形式展示日程
- **AddEventDialog**: 添加事件对话框
- **ImportProfessorDialog**: 导入教师数据对话框
- **ResultDisplayWidget**: 结果展示窗口

## Technical features

- Implement UI and data separation using the Qt Model-View framework
- Based on the C++ STL standard library
- Use `std::chrono` for time processing
- Full Chinese interface support (UTF-8 encoding)
- Modular design for easy maintenance and expansion

## License

This project is for learning and teaching purposes only.

## Contact information

If you have any questions or suggestions, please contact the development team.

---

**Happy use of the scheduling system!**

