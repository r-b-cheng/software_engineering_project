#include "MainWindow.h"
#include "ui_MainWindow.h"
#include "../modules/FileParser.h"
#include "../modules/SchedulerLogic.h"
#include <QMessageBox>
#include <QInputDialog>
#include <QFileInfo>
#include <QDir>
#include <QListWidgetItem>

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , nextEventId(1) {
    
    ui->setupUi(this);
    
    // 手动连接 ScheduleView 的删除信号
    connect(ui->scheduleView, &ScheduleView::deleteEventRequested, this, &MainWindow::onDeleteEventRequested);
    
    // 设置数据文件路径
    userDataPath = "data_storage/user_data.txt";
    professorDataPath = "data_storage/professor_data.txt";
    
    // 确保数据目录存在
    QDir dir;
    dir.mkpath("data_storage");
    
    loadData();
    updateScheduleView();
}

MainWindow::~MainWindow() {
    saveData();
    delete ui;
}



void MainWindow::loadData() {
    // 加载用户数据
    QFileInfo userFile(userDataPath);
    if (userFile.exists()) {
        User& user = dataManager.getUser();
        if (dataManager.loadUserData(user, userDataPath.toStdString())) {
            ui->statusbar->showMessage(QString::fromUtf8("用户数据已加载"), 3000);
            
            // 找到最大的事件ID
            for (const auto& event : user.getCourses().getAllEvents()) {
                if (event.getId() >= nextEventId) {
                    nextEventId = event.getId() + 1;
                }
            }
            for (const auto& event : user.getPersonalSchedule().getAllEvents()) {
                if (event.getId() >= nextEventId) {
                    nextEventId = event.getId() + 1;
                }
            }
        }
    } else {
        dataManager.getUser().setName("Student");
    }

    // 加载教师数据
    QFileInfo profFile(professorDataPath);
    if (profFile.exists()) {
        if (dataManager.loadProfessorsData(professorDataPath.toStdString())) {
            ui->statusbar->showMessage(QString::fromUtf8("教师数据已加载"), 3000);
        }
    }
}

void MainWindow::saveData() {
    dataManager.saveUserData(dataManager.getUser(), userDataPath.toStdString());
    dataManager.saveProfessorsData(dataManager.getProfessors(), professorDataPath.toStdString());
}

void MainWindow::updateScheduleView() {
    // 合并课程和个人日程
    Schedule combinedSchedule = dataManager.getUser().getCourses() +
                               dataManager.getUser().getPersonalSchedule();
    
    ui->scheduleView->setSchedule(combinedSchedule.getAllEvents());
}

// 按钮和 Action 槽函数（Qt 自动连接）
void MainWindow::on_addEventBtn_clicked() {
    AddEventDialog dialog(this);
    
    if (dialog.exec() == QDialog::Accepted) {
        ScheduleEvent event = dialog.getEvent();
        event.setId(nextEventId++);
        
        std::string errorMsg;
        bool success = false;
        
        // 根据是否为课程添加到不同的日程
        if (event.getTimeSlot().getIsCourse()) {
            success = dataManager.getUser().getCourses().addEventSafely(event, errorMsg);
        } else {
            success = dataManager.getUser().getPersonalSchedule().addEventSafely(event, errorMsg);
        }
        
        if (success) {
            updateScheduleView();
            saveData();
            QMessageBox::information(this, QString::fromUtf8("添加成功"), 
                                   QString::fromUtf8("事件已成功添加"));
        } else {
            QMessageBox::warning(this, QString::fromUtf8("添加失败"), 
                               QString::fromUtf8("无法添加事件: %1").arg(QString::fromStdString(errorMsg)));
            // 回滚ID计数
            nextEventId--;
        }
    }
}

void MainWindow::on_importStudentCoursesBtn_clicked() {
    ImportStudentCoursesDialog dialog(this);
    
    if (dialog.exec() == QDialog::Accepted) {
        QString filePath = dialog.getFilePath();
        
        if (filePath.isEmpty()) {
            QMessageBox::warning(this, QString::fromUtf8("警告"),
                               QString::fromUtf8("请选择CSV文件"));
            return;
        }
        
        try {
            // 使用FileParser解析CSV文件
            Schedule importedSchedule = FileParser::parseCsv(filePath.toStdString());
            
            // 将导入的课程添加到用户的课程日程中
            const auto& events = importedSchedule.getAllEvents();
            int successCount = 0;
            int skipCount = 0;
            
            for (const auto& event : events) {
                ScheduleEvent newEvent = event;
                newEvent.setId(nextEventId++);
                
                std::string errorMsg;
                if (dataManager.getUser().getCourses().addEventSafely(newEvent, errorMsg)) {
                    successCount++;
                } else {
                    skipCount++;
                    // 回滚ID计数
                    nextEventId--;
                }
            }
            
            updateScheduleView();
            saveData();
            
            QMessageBox::information(this, QString::fromUtf8("导入结果"),
                                   QString::fromUtf8("成功导入 %1 个课程事件，跳过 %2 个冲突或重复事件")
                                   .arg(successCount).arg(skipCount));
            
        } catch (const std::exception& e) {
            QMessageBox::critical(this, QString::fromUtf8("导入错误"),
                                QString::fromUtf8("导入失败: %1").arg(e.what()));
        }
    }
}

void MainWindow::on_importProfessorBtn_clicked() {
    ImportProfessorDialog dialog(this);
    
    if (dialog.exec() == QDialog::Accepted) {
        QString filePath = dialog.getFilePath();
        
        if (!filePath.isEmpty()) {
            try {
                std::vector<Professor> professors = FileParser::parseProfessorsCsv(filePath.toStdString());
                
                if (professors.empty()) {
                    QMessageBox::warning(this, QString::fromUtf8("导入失败"),
                                       QString::fromUtf8("未能从文件中读取教师数据"));
                    return;
                }
                
                // 合并导入的教师数据
                for (const auto& prof : professors) {
                    bool found = false;
                    for (auto& existingProf : const_cast<std::vector<Professor>&>(dataManager.getProfessors())) {
                        if (existingProf.getName() == prof.getName()) {
                            existingProf = prof;
                            found = true;
                            break;
                        }
                    }
                    if (!found) {
                        const_cast<std::vector<Professor>&>(dataManager.getProfessors()).push_back(prof);
                    }
                }
                
                saveData();
                
            } catch (const std::exception& e) {
                QMessageBox::critical(this, QString::fromUtf8("导入错误"),
                                    QString::fromUtf8("导入失败: %1").arg(e.what()));
            }
        }
    }
}

void MainWindow::on_calculateBtn_clicked() {
    const auto& professors = dataManager.getProfessors();
    
    if (professors.empty()) {
        QMessageBox::information(this, QString::fromUtf8("提示"),
                               QString::fromUtf8("请先导入教师办公时间"));
        return;
    }

    // 让用户选择教师
    QStringList profNames;
    for (const auto& prof : professors) {
        profNames << QString::fromUtf8(prof.getName().c_str());
    }

    bool ok;
    QString selectedName = QInputDialog::getItem(this,
                                                 QString::fromUtf8("选择教师"),
                                                 QString::fromUtf8("请选择要计算可用时间的教师:"),
                                                 profNames, 0, false, &ok);

    if (ok && !selectedName.isEmpty()) {
        Professor prof = dataManager.getProfessorByName(selectedName.toStdString());
        
        // 合并学生的课程和个人日程
        Schedule studentSchedule = dataManager.getUser().getCourses() +
                                  dataManager.getUser().getPersonalSchedule();

        // 取当前周偏移（来自 ScheduleView）
        int weekOffset = ui->scheduleView->getCurrentWeekOffset();
        
        // 计算可用时间段（集中使用数据层的周过滤/归一化）
        std::vector<TimeSlot> availableSlots = SchedulerLogic::findAvailableSlots(
            studentSchedule,
            prof.getOfficeHours(),
            weekOffset
        );

        // 显示结果
        ResultDisplayWidget* resultWidget = new ResultDisplayWidget(this);
        resultWidget->setResults(
            QString::fromUtf8(prof.getName().c_str()),
            QString::fromUtf8(prof.getEmail().c_str()),
            availableSlots
        );
        resultWidget->exec();
        delete resultWidget;
    }
}

void MainWindow::on_loadDataBtn_clicked() {
    loadData();
    updateScheduleView();
    QMessageBox::information(this, QString::fromUtf8("提示"),
                           QString::fromUtf8("数据已重新加载"));
}

void MainWindow::on_saveDataBtn_clicked() {
    saveData();
    QMessageBox::information(this, QString::fromUtf8("提示"),
                           QString::fromUtf8("数据已保存"));
}

void MainWindow::on_exitAction_triggered() {
    close();
}

void MainWindow::on_showScheduleAction_triggered() {
    updateScheduleView();
}


void MainWindow::onWeekChanged(int offset) {
    updateScheduleView();
}

void MainWindow::onEventDoubleClicked(int eventId) {
    showEventDetails(eventId);
}

void MainWindow::showEventDetails(int eventId) {
    // 查找事件
    ScheduleEvent* foundEvent = nullptr;
    
    for (auto& event : dataManager.getUser().getCourses().getAllEvents()) {
        if (event.getId() == eventId) {
            foundEvent = const_cast<ScheduleEvent*>(&event);
            break;
        }
    }
    
    if (!foundEvent) {
        for (auto& event : dataManager.getUser().getPersonalSchedule().getAllEvents()) {
            if (event.getId() == eventId) {
                foundEvent = const_cast<ScheduleEvent*>(&event);
                break;
            }
        }
    }

    if (foundEvent) {
        // 使用QDateTime来正确显示时间，避免std::localtime的问题
        auto startTime = std::chrono::system_clock::to_time_t(foundEvent->getTimeSlot().getStartTime());
        auto endTime = std::chrono::system_clock::to_time_t(foundEvent->getTimeSlot().getEndTime());
        
        QDateTime startDateTime = QDateTime::fromSecsSinceEpoch(startTime);
        QDateTime endDateTime = QDateTime::fromSecsSinceEpoch(endTime);
        
        QString startStr = startDateTime.toString("yyyy-MM-dd hh:mm");
        QString endStr = endDateTime.toString("yyyy-MM-dd hh:mm");
        
        QString details = QString::fromUtf8(
            "事件: %1\n"
            "地点: %2\n"
            "开始时间: %3\n"
            "结束时间: %4\n"
            "类型: %5\n"
            "备注: %6"
        ).arg(QString::fromUtf8(foundEvent->getEventName().c_str()))
         .arg(QString::fromUtf8(foundEvent->getLocation().c_str()))
         .arg(startStr)
         .arg(endStr)
         .arg(foundEvent->getTimeSlot().getIsCourse() ? QString::fromUtf8("课程") : QString::fromUtf8("个人日程"))
         .arg(QString::fromUtf8(foundEvent->getDescription().c_str()));
        
        QMessageBox::information(this, QString::fromUtf8("事件详情"), details);
    }
}

void MainWindow::onDeleteEventRequested(int eventId) {
    // 确认删除
    int ret = QMessageBox::question(this, QString::fromUtf8("确认删除"), 
                                   QString::fromUtf8("确定要删除这个事件吗？"),
                                   QMessageBox::Yes | QMessageBox::No);
    
    if (ret != QMessageBox::Yes) return;
    
    // 从课程中查找并删除
    bool found = dataManager.getUser().getCourses().removeEvent(eventId);
    
    // 如果在课程中没找到，从个人日程中查找并删除
    if (!found) {
        found = dataManager.getUser().getPersonalSchedule().removeEvent(eventId);
    }
    
    if (found) {
        updateScheduleView();
        saveData();
        QMessageBox::information(this, QString::fromUtf8("删除成功"), 
                               QString::fromUtf8("事件已删除"));
    } else {
        QMessageBox::warning(this, QString::fromUtf8("删除失败"), 
                           QString::fromUtf8("未找到指定事件"));
    }
}
