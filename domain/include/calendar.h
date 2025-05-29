#ifndef CALENDAR_H
#define CALENDAR_H

#include <vector>
#include <ctime>
#include <chrono>
#include <memory>
#include <iostream>

#include "time_window.h"

class PeriodCalendar
{
public:
    /**
     * 时间戳的范围，0-86400(60*60*24)
     */
    std::vector<std::pair<int, int>> mondays;     // 星期一
    std::vector<std::pair<int, int>> tuesdays;    // 星期二
    std::vector<std::pair<int, int>> wednesdays;  // 星期三
    std::vector<std::pair<int, int>> thursdays;   // 星期四
    std::vector<std::pair<int, int>> fridays;     // 星期五
    std::vector<std::pair<int, int>> saturdays;   // 星期六
    std::vector<std::pair<int, int>> sundays;     // 星期日

    PeriodCalendar() {}

    ~PeriodCalendar() {}

    std::vector<std::pair<int, int>> get_wday_time(int wday);
};

class Calendar
{
public:
    std::unique_ptr<PeriodCalendar> work_calendar;  // 工作日历

    Calendar(std::unique_ptr<PeriodCalendar> &work_calendar);

    ~Calendar();

    std::vector<TimeWindow *> intersection(std::unique_ptr<TimeWindowDateTime> &time_window);
};

#endif // CALENDAR_H
