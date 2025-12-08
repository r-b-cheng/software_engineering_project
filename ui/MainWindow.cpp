#include "MainWindow.h"
#include "ui_MainWindow.h"
#include "../modules/FileParser.h"
#include "../modules/SchedulerLogic.h"
#include "SettingsDialog.h"

#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QEventLoop>
#include <QMessageBox>
#include <QInputDialog>
#include <QFileInfo>
#include <QDir>
#include <set>
#include <QListWidgetItem>
#include <QCalendarWidget>
#include <QComboBox>
#include <QListWidget>
#include <QTime>
#include <algorithm>
#include <cmath>

namespace {
struct TagOption {
    QString label;
    int value;
};

const TagOption kTagOptions[] = {
    {QString::fromUtf8(u8"\u5168\u90E8\u6807\u7B7E"), TAG_NONE},
    {QString::fromUtf8(u8"\u671F\u4E2D"), TAG_MIDTERM},
    {QString::fromUtf8(u8"\u671F\u672B"), TAG_FINAL},
    {QString::fromUtf8(u8"\u590D\u4E60"), TAG_REVIEW},
    {QString::fromUtf8(u8"\u8865\u8BFE"), TAG_MAKEUP},
    {QString::fromUtf8("Pre"), TAG_PRE},
    {QString::fromUtf8(u8"\u7D27\u6025"), TAG_URGENT},
    {QString::fromUtf8(u8"\u91CD\u8981"), TAG_IMPORTANT},
};

QString weekdayLabel(int weekday) {
    static const QStringList labels = {
        QString::fromUtf8(u8"\u5468\u4E00"), QString::fromUtf8(u8"\u5468\u4E8C"), QString::fromUtf8(u8"\u5468\u4E09"),
        QString::fromUtf8(u8"\u5468\u56DB"), QString::fromUtf8(u8"\u5468\u4E94"), QString::fromUtf8(u8"\u5468\u516D"),
        QString::fromUtf8(u8"\u5468\u65E5")
    };
    if (weekday < 1 || weekday > labels.size()) {
        return QString::fromUtf8(u8"\u5468?");
    }
    return labels[weekday - 1];
}

QDateTime normalizeCourseStartToWeek(const ScheduleEvent& event, const QDate& weekStart) {
    int weekday = event.getWeekday();
    if (weekday < 1 || weekday > 7) {
        weekday = 1;
    }
    QDate targetDate = weekStart.addDays(weekday - 1);
    auto start_t = std::chrono::system_clock::to_time_t(event.getTimeSlot().getStartTime());
    QTime startTime = QDateTime::fromSecsSinceEpoch(static_cast<qint64>(start_t)).time();
    return QDateTime(targetDate, startTime);
}

QDateTime computeEndFromDuration(const ScheduleEvent& event, const QDateTime& start) {
    long long durationMinutes = event.getTimeSlot().durationMinutes();
    if (durationMinutes <= 0) {
        durationMinutes = 60;
    }
    return start.addSecs(durationMinutes * 60);
}

QString formatListText(const ScheduleEvent& event, const QDateTime& start, const QDateTime& end) {
    QString name = QString::fromUtf8(event.getEventName().c_str());
    QString location = QString::fromUtf8(event.getLocation().c_str());
    QString datePart = start.date().toString("M/d");
    QString timePart = QString("%1-%2").arg(start.time().toString("HH:mm"),
                                            end.time().toString("HH:mm"));
    QString weekday = weekdayLabel(event.getWeekday());
    QString locPart = location.isEmpty() ? QString::fromUtf8(u8"未填地点") : location;
    return QString("%1 %2 %3 | %4 @ %5").arg(datePart, weekday, timePart, name, locPart);
}
}  // namespace

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , nextEventId(1)
    , calendarSyncInProgress(false) {

    ui->setupUi(this);

    initTagFilter();

    if (ui->mainLayout) {
        ui->mainLayout->setStretch(0, 1);
        ui->mainLayout->setStretch(1, 4);
        ui->mainLayout->setStretch(2, 2);
    }

    if (ui->calendarWidget) {
        ui->calendarWidget->setMaximumWidth(320);
        connect(ui->calendarWidget, &QCalendarWidget::clicked,
                this, &MainWindow::onCalendarDateSelected);
    }

    // 手动连接 ScheduleView 的删除信号
    connect(ui->scheduleView, &ScheduleView::deleteEventRequested, this, &MainWindow::onDeleteEventRequested);
    connect(ui->scheduleView, &ScheduleView::weekChanged, this, &MainWindow::onWeekChanged);
    connect(ui->scheduleView, &ScheduleView::eventDoubleClicked, this, &MainWindow::onEventDoubleClicked);
    connect(ui->scheduleView, &ScheduleView::createEventRequested, this, &MainWindow::onCreateEventFromSelection);

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
            ui->statusbar->showMessage(QString::fromUtf8(u8"用户数据已加载"), 3000);

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

            // 加载持久化的假期与屏蔽课程信息到界面
            holidays.clear();
            for (const auto& h : dataManager.getHolidays()) {
                QDate d(h.year, h.month, h.day);
                holidays.push_back({d, QString::fromStdString(h.name)});
            }
            suppressedCourseWeeks.clear();
            for (const auto& p : dataManager.getSuppressedCourseWeeks()) {
                suppressedCourseWeeks.insert(p);
            }
            ui->scheduleView->setHolidays(filterWeekHolidays(ui->scheduleView->getCurrentWeekOffset()));
            ui->scheduleView->setSuppressedCourseWeeks(suppressedCourseWeeks);
        }
    } else {
        dataManager.getUser().setName("Student");
    }

    // 加载教师数据
    QFileInfo profFile(professorDataPath);
    if (profFile.exists()) {
        if (dataManager.loadProfessorsData(professorDataPath.toStdString())) {
            ui->statusbar->showMessage(QString::fromUtf8(u8"教师数据已加载"), 3000);
        }
    }
}

void MainWindow::saveData() {
    // 将当前的假期与屏蔽课程写回 DataManager 以便持久化
    std::vector<DataManager::HolidayItem> hitems;
    for (const auto& h : holidays) {
        DataManager::HolidayItem it{h.first.year(), h.first.month(), h.first.day(), h.second.toStdString()};
        hitems.push_back(it);
    }
    dataManager.setHolidays(hitems);
    std::vector<std::pair<int,int>> suppressedVec(suppressedCourseWeeks.begin(), suppressedCourseWeeks.end());
    dataManager.setSuppressedCourseWeeks(suppressedVec);

    dataManager.saveUserData(dataManager.getUser(), userDataPath.toStdString());
    dataManager.saveProfessorsData(dataManager.getProfessors(), professorDataPath.toStdString());
}

void MainWindow::updateScheduleView() {
    // 合并课程和个人日程
    Schedule combinedSchedule = dataManager.getUser().getCourses() +
                               dataManager.getUser().getPersonalSchedule();

    ui->scheduleView->setSchedule(combinedSchedule.getAllEvents());
    updateTagSearchResults();
}

void MainWindow::initTagFilter() {
    if (!ui->tagFilterComboBox) {
        return;
    }

    ui->tagFilterComboBox->clear();
    for (const auto& opt : kTagOptions) {
        ui->tagFilterComboBox->addItem(opt.label, opt.value);
    }

    connect(ui->tagFilterComboBox,
            static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this,
            &MainWindow::onTagFilterChanged);

    if (ui->tagResultListWidget) {
        ui->tagResultListWidget->setAlternatingRowColors(true);
        ui->tagResultListWidget->setSpacing(2);
        ui->tagResultListWidget->setWordWrap(true);
    }
}

void MainWindow::updateTagSearchResults() {
    if (!ui->tagResultListWidget || !ui->scheduleView || !ui->tagFilterComboBox) {
        return;
    }

    ui->tagResultListWidget->clear();

    const int selectedTag = ui->tagFilterComboBox->currentData().toInt();
    const int baseWeekOffset = ui->scheduleView->getCurrentWeekOffset();
    const QDate baseWeekStart = getWeekStartDate(baseWeekOffset);

    std::vector<ScheduleEvent> allEvents = dataManager.getUser().getCourses().getAllEvents();
    const auto& personalEvents = dataManager.getUser().getPersonalSchedule().getAllEvents();
    allEvents.insert(allEvents.end(), personalEvents.begin(), personalEvents.end());

    struct DisplayItem {
        ScheduleEvent event;
        QDateTime start;
        QDateTime end;
        int distance;
    };
    std::vector<DisplayItem> filtered;

    for (const auto& ev : allEvents) {
        int effectiveTags = ev.getTags();
        if (ev.getTimeSlot().getIsCourse()) {
            std::pair<int,int> tagKey{ev.getId(), baseWeekOffset};
            if (!courseTagWeeks.count(tagKey)) {
                effectiveTags = 0;
            }
            if (suppressedCourseWeeks.count({ev.getId(), baseWeekOffset})) {
                continue;
            }
        }

        if (selectedTag != TAG_NONE && (effectiveTags & selectedTag) == 0) {
            continue;
        }

        QDateTime startDT;
        QDateTime endDT;
        int eventWeekOffset = ev.getWeekOffset();

        if (ev.getTimeSlot().getIsCourse()) {
            startDT = normalizeCourseStartToWeek(ev, baseWeekStart);
            endDT = computeEndFromDuration(ev, startDT);
            eventWeekOffset = baseWeekOffset;
        } else {
            auto start_t = std::chrono::system_clock::to_time_t(ev.getTimeSlot().getStartTime());
            auto end_t = std::chrono::system_clock::to_time_t(ev.getTimeSlot().getEndTime());
            startDT = QDateTime::fromSecsSinceEpoch(static_cast<qint64>(start_t));
            endDT = QDateTime::fromSecsSinceEpoch(static_cast<qint64>(end_t));
        }

        int distance = std::abs(eventWeekOffset - baseWeekOffset);
        filtered.push_back(DisplayItem{ev, startDT, endDT, distance});
    }

    std::sort(filtered.begin(), filtered.end(), [](const DisplayItem& a, const DisplayItem& b) {
        if (a.distance != b.distance) return a.distance < b.distance;
        if (a.start != b.start) return a.start < b.start;
        return a.event.getId() < b.event.getId();
    });

    int limit = std::min<int>(10, static_cast<int>(filtered.size()));
    for (int i = 0; i < limit; ++i) {
        const auto& item = filtered[i];
        QString text = formatListText(item.event, item.start, item.end);
        QListWidgetItem* listItem = new QListWidgetItem(text);
        listItem->setData(Qt::UserRole, item.event.getId());
        listItem->setToolTip(text);
        ui->tagResultListWidget->addItem(listItem);
    }

    if (limit == 0) {
        ui->tagResultListWidget->addItem(QString::fromUtf8(u8"暂无匹配事件"));
    }
}

void MainWindow::onTagFilterChanged(int) {
    updateTagSearchResults();
}

// 按钮和 Action 槽函数（Qt 自动连接）
void MainWindow::on_addEventBtn_clicked() {
    AddEventDialog dialog(this);
    if (dialog.exec() == QDialog::Accepted) {
        ScheduleEvent event = dialog.getEvent();
        processNewEvent(event);
    }
}

void MainWindow::on_importStudentCoursesBtn_clicked() {
    ImportStudentCoursesDialog dialog(this);

    if (dialog.exec() == QDialog::Accepted) {
        QString filePath = dialog.getFilePath();

        if (filePath.isEmpty()) {
            QMessageBox::warning(this, QString::fromUtf8(u8"警告"),
                                 QString::fromUtf8(u8"请选择CSV文件"));
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

            QMessageBox::information(this, QString::fromUtf8(u8"导入结果"),
                                     QString::fromUtf8(u8"成功导入 %1 个课程事件，跳过 %2 个冲突或重复事件")
                                     .arg(successCount).arg(skipCount));

        } catch (const std::exception& e) {
            QMessageBox::critical(this, QString::fromUtf8(u8"导入错误"),
                                  QString::fromUtf8(u8"导入失败: %1").arg(e.what()));
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
                    QMessageBox::warning(this, QString::fromUtf8(u8"导入失败"),
                                         QString::fromUtf8(u8"未能从文件中读取教师数据"));
                    return;
                }

                // 合并：按姓名合并，返回值表示是否发生变更
                bool changed = dataManager.importOrMergeProfessors(professors);
                // 调用 saveData 保存教师数据到 data_storage
                if (changed) {
                    saveData();
                }
            } catch (const std::exception& e) {
                QMessageBox::critical(this, QString::fromUtf8(u8"导入错误"),
                                      QString::fromUtf8(u8"导入失败: %1").arg(e.what()));
            }
        }
    }
}

void MainWindow::on_calculateBtn_clicked() {
    const auto& professors = dataManager.getProfessors();

    if (professors.empty()) {
        QMessageBox::information(this, QString::fromUtf8(u8"提示"),
                                 QString::fromUtf8(u8"请先导入教师办公时间"));
        return;
    }

    // 让用户选择教师
    QStringList profNames;
    for (const auto& prof : professors) {
        profNames << QString::fromUtf8(prof.getName().c_str());
    }

    bool ok;
    QString selectedName = QInputDialog::getItem(this,
                                                 QString::fromUtf8(u8"选择教师"),
                                                 QString::fromUtf8(u8"请选择要计算可用时间的教师:"),
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
    QMessageBox::information(this, QString::fromUtf8(u8"提示"),
                             QString::fromUtf8(u8"数据已重新加载"));
}

void MainWindow::on_saveDataBtn_clicked() {
    saveData();
    QMessageBox::information(this, QString::fromUtf8(u8"提示"),
                             QString::fromUtf8(u8"数据已保存"));
}

void MainWindow::on_exitAction_triggered() {
    saveData();
    close();
}

void MainWindow::on_showScheduleAction_triggered() {
    updateScheduleView();
}

void MainWindow::on_settingsAction_triggered() {
    SettingsDialog dialog(this);
    dialog.setAutoJumpChecked(ui->autoJumpAction->isChecked());

    if (dialog.exec() == QDialog::Accepted) {
        bool checked = dialog.getAutoJumpChecked();
        ui->autoJumpAction->setChecked(checked);
        ui->statusbar->showMessage(QString::fromUtf8(u8"设置已更新"), 3000);
    }
}

void MainWindow::onWeekChanged(int offset) {
    updateScheduleView();
    // 更新表头节假日提示
    ui->scheduleView->setHolidays(filterWeekHolidays(offset));

    if (!calendarSyncInProgress && ui->calendarWidget) {
        QDate weekStart = getWeekStartDate(offset);
        if (weekStart.isValid()) {
            ui->calendarWidget->setSelectedDate(weekStart);
        }
    }
}

void MainWindow::onCalendarDateSelected(const QDate& date) {
    if (!date.isValid()) {
        return;
    }

    int targetOffset = getWeekOffsetForDate(date);
    if (targetOffset == ui->scheduleView->getCurrentWeekOffset()) {
        return;
    }

    calendarSyncInProgress = true;
    ui->scheduleView->setWeekOffset(targetOffset);
    calendarSyncInProgress = false;
}

void MainWindow::onEventDoubleClicked(int eventId) {
    showEventDetails(eventId);
}

void MainWindow::onCreateEventFromSelection(const QDateTime& start, const QDateTime& end) {
    AddEventDialog dialog(this);
    dialog.presetTimeRange(start, end);

    if (dialog.exec() == QDialog::Accepted) {
        ScheduleEvent event = dialog.getEvent();
        processNewEvent(event);
    }
}

void MainWindow::showEventDetails(int eventId) {
    // 查找事件（复制安全副本，课程按当前周归一化）
    bool found = false;
    ScheduleEvent foundEvent;

    const int currentOffset = ui->scheduleView->getCurrentWeekOffset();
    // 课程：使用归一化后的当周副本
    for (const auto& ev : dataManager.getUser().getCourses().getEventsForWeekCopy(currentOffset)) {
        if (ev.getId() == eventId) {
            foundEvent = ev;
            found = true;
            break;
        }
    }
    // 个人事件：直接匹配原事件
    if (!found) {
        for (const auto& ev : dataManager.getUser().getPersonalSchedule().getAllEvents()) {
            if (ev.getId() == eventId) {
                foundEvent = ev;
                found = true;
                break;
            }
        }
    }

    if (found) {
        AddEventDialog dialog(this);
        dialog.setWindowTitle(QString::fromUtf8(u8"编辑事件标签"));
        // 根据当前周偏移映射过滤课程标签的初始显示
        ScheduleEvent displayEvent = foundEvent;
        if (displayEvent.getTimeSlot().getIsCourse()) {
            int currentOffsetForUI = ui->scheduleView->getCurrentWeekOffset();
            if (courseTagWeeks.count({eventId, currentOffsetForUI}) == 0) {
                displayEvent.setTags(0);
            }
        }
        dialog.setEvent(displayEvent);
        dialog.setTagEditOnly(true);

        if (dialog.exec() == QDialog::Accepted) {
            int newTags = dialog.getSelectedTags();

            bool courseUpdated = false;

            const auto& courseEvents = dataManager.getUser().getCourses().getAllEvents();
            for (const auto& e : courseEvents) {
                if (e.getId() == eventId) {
                    ScheduleEvent updatedEvent = e;
                    updatedEvent.setTags(newTags);
                    dataManager.getUser().getCourses().removeEvent(eventId);
                    std::string err;
                    dataManager.getUser().getCourses().addEventSafely(updatedEvent, err);
                    courseUpdated = true;
                    break;
                }
            }

            bool personalUpdated = false;
            if (!courseUpdated) {
                const auto& personalEvents = dataManager.getUser().getPersonalSchedule().getAllEvents();
                for (const auto& e : personalEvents) {
                    if (e.getId() == eventId) {
                        ScheduleEvent updatedEvent = e;
                        updatedEvent.setTags(newTags);
                        dataManager.getUser().getPersonalSchedule().removeEvent(eventId);
                        std::string err;
                        dataManager.getUser().getPersonalSchedule().addEventSafely(updatedEvent, err);
                        personalUpdated = true;
                        break;
                    }
                }
            }

            if (courseUpdated) {
                if (foundEvent.getTimeSlot().getIsCourse()) {
                    int currentOffset = ui->scheduleView->getCurrentWeekOffset();
                    if (newTags != 0) {
                        courseTagWeeks.insert({eventId, currentOffset});
                    } else {
                        courseTagWeeks.erase({eventId, currentOffset});
                    }
                    ui->scheduleView->setCourseTagWeeks(courseTagWeeks);
                }
            }
            
            if (courseUpdated || personalUpdated) {
                updateScheduleView();
                saveData();
                QMessageBox::information(this, QString::fromUtf8(u8"已更新"), QString::fromUtf8(u8"标签已更新"));
            } else {
                QMessageBox::warning(this, QString::fromUtf8(u8"更新失败"), QString::fromUtf8(u8"未找到事件"));
            }
        }
    }
}

void MainWindow::processNewEvent(ScheduleEvent event) {
    event.setId(nextEventId++);

    std::string errorMsg;
    bool success = false;

    // 校正无效结束时间，避免负时长
    TimeSlot slot = event.getTimeSlot();
    if (slot.getEndTime() <= slot.getStartTime()) {
        slot.setEndTime(slot.getStartTime() + std::chrono::minutes(1));
        event.setTimeSlot(slot);
    }

    QDate startDate = QDateTime::fromSecsSinceEpoch(
        static_cast<qint64>(std::chrono::system_clock::to_time_t(slot.getStartTime()))
    ).date();

    if (slot.getIsCourse()) {
        for (const auto& h : holidays) {
            if (h.first == startDate) {
                QMessageBox::warning(this, QString::fromUtf8(u8"添加失败"),
                                     QString::fromUtf8(u8"假期当天不允许添加课程"));
                --nextEventId;
                return;
            }
        }
    }

    // 用事件自带的 weekday + TimeSlot 重叠判断
    const int newWeekday = event.getWeekday();
    for (const auto& c : dataManager.getUser().getCourses().getAllEvents()) {
        if (c.getWeekday() != newWeekday) continue;

        TimeSlot existing = c.getTimeSlot();
        if (existing.getEndTime() <= existing.getStartTime()) {
            existing.setEndTime(existing.getStartTime() + std::chrono::minutes(1));
        }

        if (slot.isOverlappingWith(existing)) {
            QMessageBox::warning(this, QString::fromUtf8(u8"时间冲突"),
                                 QString::fromUtf8(u8"与课程时间重叠，不能添加"));
            --nextEventId;
            return;
        }
    }

    if (event.getTimeSlot().getIsCourse()) {
        success = dataManager.getUser().getCourses().addEventSafely(event, errorMsg);
    } else {
        success = dataManager.getUser().getPersonalSchedule().addEventSafely(event, errorMsg);
    }

    if (success) {
        if (event.getTimeSlot().getIsCourse() && event.getTags() != 0) {
            courseTagWeeks.insert({event.getId(), event.getWeekOffset()});
            ui->scheduleView->setCourseTagWeeks(courseTagWeeks);
        }
        if (ui->autoJumpAction->isChecked()) {
            ui->scheduleView->setWeekOffset(event.getWeekOffset());
        }
        updateScheduleView();
        saveData();
        QMessageBox::information(this, QString::fromUtf8(u8"添加成功"),
                               QString::fromUtf8(u8"事件已成功添加"));
    } else {
        QMessageBox::warning(this, QString::fromUtf8(u8"添加失败"),
                           QString::fromUtf8(u8"无法添加事件: %1").arg(QString::fromStdString(errorMsg)));
        nextEventId--;
    }
}

QDate MainWindow::getWeekStartDate(int weekOffset) const {
    QDate today = QDate::currentDate();
    int daysToMonday = today.dayOfWeek() - 1;
    QDate weekStart = today.addDays(-daysToMonday);
    return weekStart.addDays(weekOffset * 7);
}

int MainWindow::getWeekOffsetForDate(const QDate& date) const {
    if (!date.isValid()) {
        return 0;
    }

    QDate targetWeekStart = date.addDays(-(date.dayOfWeek() - 1));
    QDate currentWeekStart = getWeekStartDate(0);
    int dayDiff = currentWeekStart.daysTo(targetWeekStart);
    int offset = dayDiff / 7;
    return offset;
}

void MainWindow::onDeleteEventRequested(int eventId) {
    // 确认删除
    int ret = QMessageBox::question(this, QString::fromUtf8(u8"确认删除"), 
                                   QString::fromUtf8(u8"确定要删除这条事件吗？"),
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
        QMessageBox::information(this, QString::fromUtf8(u8"删除成功"), 
                               QString::fromUtf8(u8"事件已删除"));
    } else {
        QMessageBox::warning(this, QString::fromUtf8(u8"删除失败"), 
                           QString::fromUtf8(u8"未找到指定事件"));
    }
}

std::vector<std::pair<QDate, QString>> MainWindow::filterWeekHolidays(int offset) const {
    std::vector<std::pair<QDate, QString>> result;
    QDate weekStart = getWeekStartDate(offset);
    for (const auto& h : holidays) {
        int diff = weekStart.daysTo(h.first);
        if (diff >= 0 && diff < 7) {
            result.push_back(h);
        }
    }
    return result;
}

void MainWindow::on_syncHolidaysAction_triggered() {
    const QUrl icsUrl("https://www.officeholidays.com/ics/ics_country.php?tbl_country=Macau");
    QNetworkAccessManager mgr;
    QNetworkRequest req(icsUrl);
    QEventLoop loop;
    QNetworkReply* reply = mgr.get(req);
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();

    if (reply->error() != QNetworkReply::NoError) {
        QMessageBox::warning(this, QString::fromUtf8(u8"同步失败"),
                             QString::fromUtf8(u8"无法获取假期数据: %1").arg(reply->errorString()));
        reply->deleteLater();
        return;
    }

    QByteArray data = reply->readAll();
    reply->deleteLater();

    std::vector<ScheduleEvent> holidaysEvents = FileParser::parseIcsHolidays(std::string(data.constData(), data.size()));
    if (holidaysEvents.empty()) {
        QMessageBox::information(this, QString::fromUtf8(u8"同步结果"), QString::fromUtf8(u8"未解析到假期事件"));
        return;
    }

    // 转存为日期+名称，并删除当天的课程事件
    std::set<QDate> holidayDates;
    holidays.clear();
    for (const auto& e : holidaysEvents) {
        auto st = std::chrono::system_clock::to_time_t(e.getTimeSlot().getStartTime());
        QDate d = QDateTime::fromSecsSinceEpoch(static_cast<qint64>(st)).date();
        holidays.push_back({d, QString::fromStdString(e.getEventName())});
        holidayDates.insert(d);
    }

    // 非破坏性处理：仅屏蔽节假日所在周的课程出现，不全局删除
    for (const QDate& hd : holidayDates) {
        int weekOffset = getWeekOffsetForDate(hd);
        for (const auto& ev : dataManager.getUser().getCourses().getAllEvents()) {
            if (ev.getWeekday() == hd.dayOfWeek()) {
                suppressedCourseWeeks.insert({ev.getId(), weekOffset});
            }
        }
    }

    updateScheduleView();
    ui->scheduleView->setSuppressedCourseWeeks(suppressedCourseWeeks);
    ui->scheduleView->setHolidays(filterWeekHolidays(ui->scheduleView->getCurrentWeekOffset()));
    saveData();
    QMessageBox::information(this, QString::fromUtf8(u8"同步完成"),
                             QString::fromUtf8(u8"假期当周的课程已屏蔽"));
}

void MainWindow::on_clearHolidaysAction_triggered() {
    // 删除个人日程中描述为“公共假期”的事件（之前同步的假期事件）
    std::vector<int> toRemove;
    for (const auto& e : dataManager.getUser().getPersonalSchedule().getAllEvents()) {
        if (QString::fromStdString(e.getDescription()) == QString::fromUtf8(u8"公共假期")) {
            toRemove.push_back(e.getId());
        }
    }
    for (int id : toRemove) {
        dataManager.getUser().getPersonalSchedule().removeEvent(id);
    }
    updateScheduleView();
    saveData();
    QMessageBox::information(this, QString::fromUtf8(u8"清理完成"),
                             QString::fromUtf8(u8"已删除旧假期事件 %1 条").arg(static_cast<int>(toRemove.size())));
}
