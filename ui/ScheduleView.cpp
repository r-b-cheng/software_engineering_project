#include "ScheduleView.h"
#include <QHeaderView>
#include <QMessageBox>
#include <QDate>
#include <QDebug>
#include <QMenu>
#include <QAction>
#include <QTime>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <QMouseEvent>
#include <algorithm>
#include <cmath>

#include "ScheduleCellDelegate.h"

namespace {
constexpr int kSlotMinutes = 30;
constexpr int kSlotsPerHour = 60 / kSlotMinutes;
constexpr int kSlotsPerDay = 24 * kSlotsPerHour;
constexpr int kSecondsPerSlot = kSlotMinutes * 60;
}

ScheduleView::ScheduleView(QWidget* parent)
    : QWidget(parent),
      tableView(nullptr),
      model(nullptr),
      prevWeekButton(nullptr),
      nextWeekButton(nullptr),
      weekLabel(nullptr),
      currentWeekOffset(0),
      hoverLabel(nullptr),
      currentEvents(),
      occupiedSlots(kSlotsPerDay, std::vector<bool>(8, false)),
      dragSelecting(false),
      dragStartTime(),
      dragStartSlot(-1),
      dragStartColumn(-1) {
    setupUI();
}

ScheduleView::~ScheduleView() {
}

void ScheduleView::setupUI() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    // 周切换控制栏
    QHBoxLayout* controlLayout = new QHBoxLayout();
    prevWeekButton = new QPushButton(QString::fromUtf8("上一周"), this);
    nextWeekButton = new QPushButton(QString::fromUtf8("下一周"), this);
    weekLabel = new QLabel(QString::fromUtf8("当前周"), this);
    weekLabel->setAlignment(Qt::AlignCenter);

    controlLayout->addWidget(prevWeekButton);
    controlLayout->addWidget(weekLabel, 1);
    controlLayout->addWidget(nextWeekButton);

    // 创建表格视图
    tableView = new QTableView(this);
    model = new QStandardItemModel(24, 8, this);  // 24小时 x 8列（时间+7天）

    // 设置表头
    QStringList headers;
    headers << QString::fromUtf8("时间");
    
    // 添加带日期的星期列头
    QStringList weekHeaders = getWeekHeaders();
    for (int i = 0; i < 7; ++i) {
        headers << weekHeaders[i];
    }
    
    model->setHorizontalHeaderLabels(headers);

    // 填充时间列
    for (int i = 0; i < 24; ++i) {
        QString timeStr = QString("%1:00").arg(i, 2, 10, QChar('0'));
        model->setItem(i, 0, new QStandardItem(timeStr));
    }

    tableView->setModel(model);
    tableView->setSelectionMode(QAbstractItemView::ContiguousSelection);
    tableView->setSelectionBehavior(QAbstractItemView::SelectItems);
    tableView->setMouseTracking(true);
    tableView->viewport()->installEventFilter(this);
    tableView->horizontalHeader()->setStretchLastSection(false);
    tableView->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    tableView->horizontalHeader()->setFixedHeight(70);
    tableView->horizontalHeader()->setSectionsClickable(true);
    tableView->verticalHeader()->setVisible(false);
    tableView->verticalHeader()->setDefaultSectionSize(44);
    tableView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    tableView->setContextMenuPolicy(Qt::CustomContextMenu);
    tableView->setStyleSheet("QTableView { gridline-color: #e0e0e0; }");

    // 布局
    mainLayout->addLayout(controlLayout);
    mainLayout->addWidget(tableView);

    setLayout(mainLayout);

    // 连接信号
    connect(prevWeekButton, &QPushButton::clicked, this, &ScheduleView::onPrevWeekClicked);
    connect(nextWeekButton, &QPushButton::clicked, this, &ScheduleView::onNextWeekClicked);
    connect(tableView, &QTableView::doubleClicked, this, &ScheduleView::onCellDoubleClicked);
    connect(tableView, &QTableView::customContextMenuRequested, this, &ScheduleView::onContextMenuRequested);
    connect(tableView->horizontalHeader(), &QHeaderView::sectionDoubleClicked, this, &ScheduleView::onHeaderDoubleClicked);

    tableView->setItemDelegate(new ScheduleCellDelegate(tableView));

    hoverLabel = new QLabel(tableView->viewport());
    hoverLabel->setStyleSheet("background-color: rgba(0, 0, 0, 160); color: white; border-radius: 4px; padding: 2px 6px; font-size: 10px;");
    hoverLabel->setAlignment(Qt::AlignCenter);
    hoverLabel->setAttribute(Qt::WA_TransparentForMouseEvents);
    hoverLabel->hide();

    updateWeekLabel();
}

void ScheduleView::updateWeekLabel() {
    if (currentWeekOffset == 0) {
        weekLabel->setText(QString::fromUtf8("本周"));
    } else if (currentWeekOffset > 0) {
        weekLabel->setText(QString::fromUtf8("未来第 %1 周").arg(currentWeekOffset));
    } else {
        weekLabel->setText(QString::fromUtf8("过去第 %1 周").arg(-currentWeekOffset));
    }
}

void ScheduleView::setWeekOffset(int offset) {
    currentWeekOffset = offset;
    updateWeekLabel();
    
    // 更新表头以反映新的周
    QStringList headers;
    headers << QString::fromUtf8("时间");
    QStringList weekHeaders = getWeekHeaders();
    for (int i = 0; i < 7; ++i) {
        headers << weekHeaders[i];
    }
    model->setHorizontalHeaderLabels(headers);
    
    // 更新tooltip和字体大小
    updateHeaderTooltips();
    adjustHeaderFontSize();
    
    emit weekChanged(offset);
}

void ScheduleView::setSchedule(const std::vector<ScheduleEvent>& events) {
    currentEvents = events;
    
    // 清空表格（除了时间列）
    for (int row = 0; row < 24; ++row) {
        for (int col = 1; col <= 7; ++col) {
            model->setItem(row, col, new QStandardItem(""));
        }
    }
    for (auto& rowSlots : occupiedSlots) {
        std::fill(rowSlots.begin(), rowSlots.end(), false);
    }
    
    // 重置所有合并的单元格
    tableView->clearSpans();

    // 填充事件
    for (const auto& event : events) {
        if (!event.getTimeSlot().getIsCourse() && event.getWeekOffset() != currentWeekOffset) {
            continue;
        }
        if (event.getTimeSlot().getIsCourse()) {
            std::pair<int,int> key{event.getId(), currentWeekOffset};
            if (suppressedCourseWeeks.count(key)) {
                continue;
            }
        }
        
        int weekday = event.getWeekday();
        if (weekday < 1 || weekday > 7) continue;

        auto startTime = std::chrono::system_clock::to_time_t(event.getTimeSlot().getStartTime());
        std::tm* start_tm = std::localtime(&startTime);
        if (!start_tm) {
            continue;
        }

        int startHour = start_tm->tm_hour;
        int startMinute = start_tm->tm_min;
        if (startHour < 0 || startHour >= 24) continue;

        int normalizedMinute = (startMinute / kSlotMinutes) * kSlotMinutes;
        normalizedMinute = std::clamp(normalizedMinute, 0, 60 - kSlotMinutes);
        int startSlot = startHour * kSlotsPerHour + (normalizedMinute / kSlotMinutes);

        auto duration = std::chrono::duration_cast<std::chrono::minutes>(
            event.getTimeSlot().getEndTime() - event.getTimeSlot().getStartTime());
        long long durationMinutes = duration.count();
        if (durationMinutes <= 0) {
            durationMinutes = kSlotMinutes;
        }
        int durationSlots = static_cast<int>((durationMinutes + kSlotMinutes - 1) / kSlotMinutes);
        durationSlots = std::min(durationSlots, kSlotsPerDay - startSlot);
        if (durationSlots <= 0) {
            continue;
        }

        int spanRows = static_cast<int>(std::ceil((normalizedMinute + durationMinutes) / 60.0));
        spanRows = std::max(1, spanRows);
        spanRows = std::min(spanRows, 24 - startHour);
        if (spanRows <= 0) {
            spanRows = 1;
        }

        QString displayText = QString::fromUtf8(event.getEventName().c_str());

        QStandardItem* item = new QStandardItem(displayText);
        item->setData(static_cast<int>(Qt::AlignLeft | Qt::AlignTop), Qt::TextAlignmentRole);
        item->setData(event.getId(), Qt::UserRole);
        item->setData(normalizedMinute, ScheduleRoles::StartMinuteRole);
        item->setData(static_cast<int>(durationMinutes), ScheduleRoles::DurationMinutesRole);
        item->setData(spanRows, ScheduleRoles::SpanRowsRole);
        int effectiveTags = event.getTags();
        if (event.getTimeSlot().getIsCourse()) {
            std::pair<int,int> tagKey{event.getId(), currentWeekOffset};
            if (!courseTagWeeks.count(tagKey)) {
                effectiveTags = 0;
            }
        }
        item->setData(effectiveTags, ScheduleRoles::TagsRole);
        QColor eventColor = event.getTimeSlot().getIsCourse() ? QColor(173, 216, 230)
                                                              : QColor(255, 255, 224);
        item->setData(eventColor, ScheduleRoles::ColorRole);

        model->setItem(startHour, weekday, item);
        for (int span = 0; span < durationSlots && (startSlot + span) < kSlotsPerDay; ++span) {
            if (weekday < static_cast<int>(occupiedSlots[startSlot + span].size())) {
                occupiedSlots[startSlot + span][weekday] = true;
            }
        }
        
        if (spanRows > 1) {
            tableView->setSpan(startHour, weekday, spanRows, 1);
        }
    }

    // 更新表头以附加节假日提示
    QStringList headers;
    headers << QString::fromUtf8("时间");
    QStringList weekHeaders = getWeekHeaders();
    for (int i = 0; i < 7; ++i) headers << weekHeaders[i];
    model->setHorizontalHeaderLabels(headers);
}

void ScheduleView::setSuppressedCourseWeeks(const std::set<std::pair<int,int>>& suppressed) {
    suppressedCourseWeeks = suppressed;
    // 触发表头与视图刷新
    QStringList headers;
    headers << QString::fromUtf8("时间");
    QStringList weekHeaders = getWeekHeaders();
    for (int i = 0; i < 7; ++i) headers << weekHeaders[i];
    model->setHorizontalHeaderLabels(headers);
    // 重绘当前周内容
    setSchedule(currentEvents);
}

void ScheduleView::setCourseTagWeeks(const std::set<std::pair<int,int>>& courseTags) {
    courseTagWeeks = courseTags;
    // 重绘当前周内容以应用标签显示规则
    setSchedule(currentEvents);
}

int ScheduleView::getCurrentWeekOffset() const {
    return currentWeekOffset;
}

void ScheduleView::onPrevWeekClicked() {
    setWeekOffset(currentWeekOffset - 1);
}

void ScheduleView::onNextWeekClicked() {
    setWeekOffset(currentWeekOffset + 1);
}

void ScheduleView::onCellDoubleClicked(const QModelIndex& index) {
    if (!index.isValid() || index.column() == 0) return;

    QStandardItem* item = model->itemFromIndex(index);
    if (item && item->text().isEmpty()) return;

    int eventId = item->data(Qt::UserRole).toInt();
    if (eventId > 0) {
        emit eventDoubleClicked(eventId);
    }
}

QStringList ScheduleView::getWeekHeaders() {
    QStringList headers;
    QStringList weekNames = {QString::fromUtf8("周一"), QString::fromUtf8("周二"), 
                            QString::fromUtf8("周三"), QString::fromUtf8("周四"), 
                            QString::fromUtf8("周五"), QString::fromUtf8("周六"), 
                            QString::fromUtf8("周日")};
    
    // 获取当前日期
    QDate today = QDate::currentDate();
    
    // 计算当前周的开始日期（周一）
    int daysToMonday = today.dayOfWeek() - 1;  // Qt中周一=1，所以减1
    QDate weekStart = today.addDays(-daysToMonday);
    
    // 根据周偏移量调整
    QDate targetWeekStart = weekStart.addDays(currentWeekOffset * 7);
    
    // 生成一周的日期，附加外部节假日提示（不以事件形式存在）
    for (int i = 0; i < 7; ++i) {
        QDate currentDate = targetWeekStart.addDays(i);
        QString base = QString("%1\n(%2/%3)")
                        .arg(weekNames[i], QString::number(currentDate.month()), QString::number(currentDate.day()));

        QStringList notes;
        for (const auto& h : weekHolidays) {
            if (h.first == currentDate) {
                notes << h.second;
            }
        }
        if (!notes.isEmpty()) {
            base += QString("\n%1").arg(QString::fromUtf8("节假日"));
        }
        headers << base;
    }
    
    return headers;
}

void ScheduleView::setHolidays(const std::vector<std::pair<QDate, QString>>& holidays) {
    weekHolidays = holidays;
    QStringList headers;
    headers << QString::fromUtf8("时间");
    QStringList weekHeaders = getWeekHeaders();
    for (int i = 0; i < 7; ++i) headers << weekHeaders[i];
    model->setHorizontalHeaderLabels(headers);
    
    // 设置表头tooltip，显示完整节假日信息
    updateHeaderTooltips();
    
    // 智能调整表头字体，只在必要时缩小
    adjustHeaderFontSize();
}

void ScheduleView::adjustHeaderFontSize() {
    QHeaderView* header = tableView->horizontalHeader();
    if (!header) return;
    // 保持默认字体大小，不因节假日名称长度而缩小
}

void ScheduleView::updateHeaderTooltips() {
    QHeaderView* header = tableView->horizontalHeader();
    if (!header) return;
    
    // 为第一列（时间列）设置tooltip
    header->model()->setHeaderData(0, Qt::Horizontal, 
        QString::fromUtf8("时间槽"), Qt::ToolTipRole);
    
    // 为每一天的列设置tooltip
    QDate weekStart = QDate::currentDate().addDays(-QDate::currentDate().dayOfWeek() + 1);
    QDate targetWeekStart = weekStart.addDays(currentWeekOffset * 7);
    
    QStringList weekNames = {QString::fromUtf8("周一"), QString::fromUtf8("周二"), 
                            QString::fromUtf8("周三"), QString::fromUtf8("周四"), 
                            QString::fromUtf8("周五"), QString::fromUtf8("周六"), 
                            QString::fromUtf8("周日")};
    for (int i = 0; i < 7; ++i) {
        QDate currentDate = targetWeekStart.addDays(i);
        QString tooltip = QString("%1 (%2/%3)")
            .arg(weekNames[i], QString::number(currentDate.month()), QString::number(currentDate.day()));
        
        // 查找该日期的所有节假日
        QStringList holidaysOnDate;
        for (const auto& h : weekHolidays) {
            if (h.first == currentDate) {
                holidaysOnDate << h.second;
            }
        }
        
        if (!holidaysOnDate.isEmpty()) {
            tooltip += QString("\n\n") + QString::fromUtf8("节假日：") + QString("\n");
            tooltip += holidaysOnDate.join(QString("\n"));
        }
        
        header->model()->setHeaderData(i + 1, Qt::Horizontal, tooltip, Qt::ToolTipRole);
    }
}

void ScheduleView::onContextMenuRequested(const QPoint& pos) {
    QModelIndex index = tableView->indexAt(pos);
    if (!index.isValid() || index.column() == 0) return;

    QStandardItem* item = model->itemFromIndex(index);
    if (!item || item->text().isEmpty()) return;

    int eventId = item->data(Qt::UserRole).toInt();
    if (eventId <= 0) return;

    QMenu contextMenu(this);
    QAction* deleteAction = contextMenu.addAction(QString::fromUtf8("删除事件"));
    
    QAction* selectedAction = contextMenu.exec(tableView->mapToGlobal(pos));
    if (selectedAction == deleteAction) {
        emit deleteEventRequested(eventId);
    }
}

void ScheduleView::onHeaderDoubleClicked(int section) {
    if (section < 1 || section > 7) return;
    QDate d = getDateForColumn(section);
    if (!d.isValid()) return;
    QStringList notes;
    for (const auto& h : weekHolidays) {
        if (h.first == d) notes << h.second;
    }
    QString msg;
    if (notes.isEmpty()) {
        msg = QString::fromUtf8("日期: %1\n无节假日").arg(d.toString("yyyy年MM月dd日"));
    } else {
        msg = QString::fromUtf8("日期: %1\n\n节假日：\n%2").arg(d.toString("yyyy年MM月dd日")).arg(notes.join(QString::fromUtf8("\n")));
    }
    QMessageBox::information(this, QString::fromUtf8("节假日详情"), msg);
}

QDate ScheduleView::getDateForColumn(int column) const {
    if (column < 1 || column > 7) {
        return {};
    }

    QDate today = QDate::currentDate();
    int daysToMonday = today.dayOfWeek() - 1;
    QDate weekStart = today.addDays(-daysToMonday);
    QDate targetWeekStart = weekStart.addDays(currentWeekOffset * 7);
    return targetWeekStart.addDays(column - 1);
}

void ScheduleView::beginSelection(const QPoint& pos) {
    QDateTime time;
    int slot = -1;
    int column = -1;
    dragSelecting = computeTimeAtPosition(pos, time, slot, column);
    if (dragSelecting) {
        bool hasEvent = false;
        if (tableView && model) {
            QModelIndex index = tableView->indexAt(pos);
            if (index.isValid() && index.column() > 0) {
                if (QStandardItem* item = model->itemFromIndex(index)) {
                    int eventId = item->data(Qt::UserRole).toInt();
                    if (eventId > 0 || !item->text().isEmpty()) {
                        hasEvent = true;
                    }
                }
            }
        }
        if (!hasEvent && slot >= 0 && slot < static_cast<int>(occupiedSlots.size()) &&
            column >= 1 && column < static_cast<int>(occupiedSlots[slot].size())) {
            hasEvent = occupiedSlots[slot][column];
        }

        if (hasEvent) {
            dragSelecting = false;
            dragStartSlot = -1;
            dragStartColumn = -1;
            return;
        }

        dragStartTime = time;
        dragStartSlot = slot;
        dragStartColumn = column;
    } else {
        dragStartSlot = -1;
        dragStartColumn = -1;
    }
}

void ScheduleView::finalizeSelection(const QPoint& pos) {
    if (!dragSelecting) {
        return;
    }
    dragSelecting = false;

    QDateTime releaseTime;
    int releaseSlot = -1;
    int releaseColumn = -1;
    if (!computeTimeAtPosition(pos, releaseTime, releaseSlot, releaseColumn)) {
        return;
    }

    if (releaseColumn != dragStartColumn) {
        return;
    }

    int startSlot = dragStartSlot;
    int endSlot = releaseSlot;
    QDateTime startTime = dragStartTime;
    QDateTime endTime = releaseTime;

    if (endSlot < startSlot) {
        std::swap(startSlot, endSlot);
        std::swap(startTime, endTime);
    }

    if (endSlot == startSlot) {
        endSlot = startSlot + 1;
        endTime = startTime.addSecs(kSecondsPerSlot);
    }

    endSlot = std::min(endSlot, kSlotsPerDay);

    if (!isRangeAvailable(dragStartColumn, startSlot, endSlot)) {
        return;
    }

    if (tableView && tableView->selectionModel()) {
        tableView->selectionModel()->clearSelection();
    }

    endTime = startTime.addSecs((endSlot - startSlot) * kSecondsPerSlot);
    emit createEventRequested(startTime, endTime);
}

void ScheduleView::updateHoverIndicator(const QPoint& pos, bool forceHide) {
    if (!hoverLabel) {
        return;
    }

    if (forceHide || !dragSelecting || dragStartSlot < 0 || dragStartColumn < 1) {
        hoverLabel->hide();
        return;
    }

    if (!tableView || !model) {
        hoverLabel->hide();
        return;
    }

    QDateTime cursorTime;
    int cursorSlot = -1;
    int cursorColumn = -1;
    if (!computeTimeAtPosition(pos, cursorTime, cursorSlot, cursorColumn)) {
        hoverLabel->hide();
        return;
    }
    Q_UNUSED(cursorColumn);

    QDateTime selectionStartTime = dragStartTime;
    QDateTime selectionEndTime = cursorTime;
    int selectionStartSlot = dragStartSlot;
    int selectionEndSlot = cursorSlot;

    if (selectionEndSlot < selectionStartSlot) {
        std::swap(selectionStartTime, selectionEndTime);
        std::swap(selectionStartSlot, selectionEndSlot);
    }

    if (selectionEndSlot == selectionStartSlot) {
        selectionEndSlot = selectionStartSlot + 1;
    }
    selectionEndSlot = std::min(selectionEndSlot, kSlotsPerDay);

    int durationSlots = selectionEndSlot - selectionStartSlot;
    if (durationSlots <= 0) {
        hoverLabel->hide();
        return;
    }

    selectionEndTime = selectionStartTime.addSecs(durationSlots * kSecondsPerSlot);

    const QString text = QString("%1 - %2")
                             .arg(selectionStartTime.toString("hh:mm"))
                             .arg(selectionEndTime.toString("hh:mm"));

    const int targetColumn = dragStartColumn;
    if (targetColumn >= model->columnCount()) {
        hoverLabel->hide();
        return;
    }

    auto slotToViewportY = [&](int slotIndex, int& y) -> bool {
        if (slotIndex < 0) {
            return false;
        }
        if (slotIndex >= kSlotsPerDay) {
            QModelIndex lastIndex = model->index(model->rowCount() - 1, targetColumn);
            QRect lastRect = tableView->visualRect(lastIndex);
            if (!lastRect.isValid()) {
                return false;
            }
            y = lastRect.bottom();
            return true;
        }

        int row = slotIndex / kSlotsPerHour;
        if (row >= model->rowCount()) {
            return false;
        }
        int slotWithinHour = slotIndex % kSlotsPerHour;
        QModelIndex idx = model->index(row, targetColumn);
        QRect rect = tableView->visualRect(idx);
        if (!rect.isValid()) {
            return false;
        }
        int slotHeight = rect.height() / kSlotsPerHour;
        y = rect.top() + slotWithinHour * slotHeight;
        return true;
    };

    int top = 0;
    int bottom = 0;
    if (!slotToViewportY(selectionStartSlot, top) || !slotToViewportY(selectionEndSlot, bottom) || bottom <= top) {
        hoverLabel->hide();
        return;
    }

    int columnX = tableView->columnViewportPosition(targetColumn);
    int columnWidth = tableView->columnWidth(targetColumn);
    if (columnX < 0 || columnWidth <= 0) {
        hoverLabel->hide();
        return;
    }

    QRect overlayRect(columnX, top, columnWidth, bottom - top);
    overlayRect = overlayRect.adjusted(2, 2, -2, -2);

    hoverLabel->setText(text);
    hoverLabel->setGeometry(overlayRect);
    hoverLabel->raise();
    hoverLabel->show();
}

bool ScheduleView::computeTimeAtPosition(const QPoint& pos, QDateTime& time, int& slot, int& column) const {
    if (!tableView) {
        return false;
    }

    QModelIndex index = tableView->indexAt(pos);
    if (!index.isValid() || index.column() == 0) {
        return false;
    }

    column = index.column();
    int hour = index.row();
    if (hour < 0 || hour >= 24) {
        return false;
    }

    QRect rect = tableView->visualRect(index);
    if (!rect.isValid() || rect.height() == 0) {
        return false;
    }

    int localY = pos.y() - rect.top();
    int minute = localY < rect.height() / 2 ? 0 : kSlotMinutes;
    slot = hour * kSlotsPerHour + (minute / kSlotMinutes);

    QDate date = getDateForColumn(column);
    if (!date.isValid()) {
        return false;
    }

    time = QDateTime(date, QTime(hour, minute));
    return true;
}

bool ScheduleView::isRangeAvailable(int column, int startSlot, int endSlot) const {
    if (column < 1 || column > 7) {
        return false;
    }
    startSlot = std::clamp(startSlot, 0, kSlotsPerDay - 1);
    endSlot = std::clamp(endSlot, startSlot + 1, kSlotsPerDay);

    for (int slot = startSlot; slot < endSlot; ++slot) {
        if (column < static_cast<int>(occupiedSlots[slot].size()) && occupiedSlots[slot][column]) {
            return false;
        }
    }

    return true;
}

bool ScheduleView::eventFilter(QObject* watched, QEvent* event) {
    if (watched == tableView->viewport()) {
        if (event->type() == QEvent::MouseMove) {
            auto mouseEvent = static_cast<QMouseEvent*>(event);
            updateHoverIndicator(mouseEvent->pos());
        } else if (event->type() == QEvent::MouseButtonPress) {
            auto mouseEvent = static_cast<QMouseEvent*>(event);
            if (mouseEvent->button() == Qt::LeftButton) {
                beginSelection(mouseEvent->pos());
                updateHoverIndicator(mouseEvent->pos());
            }
        } else if (event->type() == QEvent::MouseButtonRelease) {
            auto mouseEvent = static_cast<QMouseEvent*>(event);
            if (mouseEvent->button() == Qt::LeftButton) {
                finalizeSelection(mouseEvent->pos());
                updateHoverIndicator(mouseEvent->pos(), true);
            }
        } else if (event->type() == QEvent::Leave) {
            updateHoverIndicator(QPoint(), true);
        }
    }

    return QWidget::eventFilter(watched, event);
}

