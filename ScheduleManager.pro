QT += core gui widgets

CONFIG += c++17

TARGET = ScheduleManager
TEMPLATE = app

# 输出目录
DESTDIR = bin
OBJECTS_DIR = build/obj
MOC_DIR = build/moc
RCC_DIR = build/rcc
UI_DIR = build/ui

# 源文件
SOURCES += \
    main.cpp \
    datastructure/TimeSlot.cpp \
    datastructure/ScheduleEvent.cpp \
    datastructure/Schedule.cpp \
    datastructure/Professor.cpp \
    datastructure/User.cpp \
    modules/DataManager.cpp \
    modules/FileParser.cpp \
    modules/SchedulerLogic.cpp \
    ui/MainWindow.cpp \
    ui/ScheduleCellDelegate.cpp \
    ui/ScheduleView.cpp \
    ui/AddEventDialog.cpp \
    ui/ImportProfessorDialog.cpp \
    ui/ImportStudentCoursesDialog.cpp \
    ui/SettingsDialog.cpp \
    ui/ResultDisplayWidget.cpp

# 头文件
HEADERS += \
    datastructure/TimeSlot.h \
    datastructure/ScheduleEvent.h \
    datastructure/Schedule.h \
    datastructure/Professor.h \
    datastructure/User.h \
    modules/DataManager.h \
    modules/FileParser.h \
    modules/SchedulerLogic.h \
    ui/MainWindow.h \
    ui/ScheduleCellDelegate.h \
    ui/ScheduleView.h \
    ui/AddEventDialog.h \
    ui/ImportProfessorDialog.h \
    ui/ImportStudentCoursesDialog.h \
    ui/SettingsDialog.h \
    ui/ResultDisplayWidget.h

# UI 文件
FORMS += \
    ui/forms/MainWindow.ui \
    ui/forms/AddEventDialog.ui \
    ui/forms/ImportProfessorDialog.ui \
    ui/forms/ImportStudentCoursesDialog.ui \
    ui/forms/SettingsDialog.ui \
    ui/forms/ResultDisplayWidget.ui

# 包含路径
INCLUDEPATH += . datastructure modules ui

# 编译器选项
QMAKE_CXXFLAGS += -std=c++17

# Windows特定配置
win32 {
    exists(icon.ico): RC_ICONS = icon.ico
}

# macOS特定配置
macx {
    ICON = icon.icns
}

# 默认规则
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

RESOURCES += resources.qrc

