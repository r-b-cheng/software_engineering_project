#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QListWidgetItem>
#include <QDateTime>
#include "ScheduleView.h"
#include "AddEventDialog.h"
#include "ImportProfessorDialog.h"
#include "ImportStudentCoursesDialog.h"
#include "ResultDisplayWidget.h"
#include "../modules/DataManager.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow();

private slots:
    // 菜单和工具栏 Action 槽函数（Qt 自动连接）
    void on_addEventBtn_clicked();
    void on_importProfessorBtn_clicked();
    void on_importStudentCoursesBtn_clicked();
    void on_calculateBtn_clicked();
    void on_loadDataBtn_clicked();
    void on_saveDataBtn_clicked();
    void on_exitAction_triggered();
    void on_showScheduleAction_triggered();
    void on_settingsAction_triggered();
    
    // ScheduleView 相关槽函数（需要手动连接，因为是自定义信号）
    void onWeekChanged(int offset);
    void onEventDoubleClicked(int eventId);
    void onDeleteEventRequested(int eventId);
    void onCreateEventFromSelection(const QDateTime& start, const QDateTime& end);
    void onCalendarDateSelected(const QDate& date);

private:
    Ui::MainWindow *ui;
    
    // 数据管理
    DataManager dataManager;
    int nextEventId;
    bool calendarSyncInProgress;
    
    // 数据文件路径
    QString userDataPath;
    QString professorDataPath;
    
    // 辅助函数
    void loadData();
    void saveData();
    void updateScheduleView();
    void showEventDetails(int eventId);
    void processNewEvent(ScheduleEvent event);
    QDate getWeekStartDate(int weekOffset) const;
    int getWeekOffsetForDate(const QDate& date) const;
};

#endif // MAINWINDOW_H
