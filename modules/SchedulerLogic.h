#ifndef SCHEDULERLOGIC_H
#define SCHEDULERLOGIC_H

#include "../datastructure/Schedule.h"
#include "../datastructure/TimeSlot.h"
#include <vector>

class SchedulerLogic {
public:
    static std::vector<TimeSlot> findAvailableSlots(
        const Schedule& studentSchedule,
        const Schedule& officeHour,
        int weekOffset);
};

#endif // SCHEDULERLOGIC_H

