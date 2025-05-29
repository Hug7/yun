// #include <cmath>

#include "time_utils.h"
#include "calendar.h"

std::vector<std::pair<int, int>> PeriodCalendar::get_wday_time(int wday)
{
    switch (wday)
    {
    case 0:
        return this->mondays;
    case 1:
        return this->tuesdays;
    case 2:
        return this->wednesdays;
    case 3:
        return this->thursdays;
    case 4:
        return this->fridays;
    case 5:
        return this->saturdays;
    case 6:
        return this->sundays;
    default:
        throw std::invalid_argument("Invalid weekday");
    }
}

Calendar::Calendar(std::unique_ptr<PeriodCalendar> &work_calendar)
{
    this->work_calendar = std::move(work_calendar);
}

Calendar::~Calendar() {}

std::vector<TimeWindow *> Calendar::intersection(std::unique_ptr<TimeWindowDateTime> &time_window)
{
    std::vector<TimeWindow *> intersection_time_windows;
    // std::time_t early_timestamp = std::mktime(&time_window->early);
    // std::time_t late_timestamp = std::mktime(&time_window->late);

    long early_timestamp = static_cast<long>(std::mktime(&time_window->early));
    long late_timestamp = static_cast<long>(std::mktime(&time_window->late));

    std::tm early_date = get_date_tm(time_window->early);
    // std::time_t early_date_timestamp = std::mktime(&early_date);
    long early_date_timestamp = static_cast<long>(std::mktime(&early_date));

    std::tm late_date = get_date_tm(time_window->late);
    // std::time_t late_date_timestamp = std::mktime(&late_date);
    long late_date_timestamp = static_cast<long>(std::mktime(&late_date));

    std::vector<std::pair<int, int>> first_day_timestamp_range = this->work_calendar->get_wday_time(early_date.tm_wday);
    // 处理第一天
    for (auto &time_range : first_day_timestamp_range)
    {
        long start_time = early_date_timestamp + time_range.first;
        long end_time = early_date_timestamp + time_range.second;
        if (end_time < early_timestamp || start_time > late_timestamp)
        {
            continue;
        }
        intersection_time_windows.push_back(new TimeWindow(std::max(start_time, early_timestamp), std::min(end_time, late_timestamp)));
    }

    // int day_diff = std::difftime(late_date_timestamp, early_date_timestamp) / 86400;
    int day_diff = (late_date_timestamp - early_date_timestamp) / 86400;
    for (int d = 1; d < day_diff; d++)
    {
        std::tm next_day_datetime = plus_day(early_date, d);
        // std::time_t next_day_timestamp = std::mktime(&next_day_datetime);
        long next_day_timestamp = static_cast<long>(std::mktime(&next_day_datetime));
        std::vector<std::pair<int, int>> timestamp_range = this->work_calendar->get_wday_time(next_day_datetime.tm_wday);
        for (auto &time_range : timestamp_range)
        {
            intersection_time_windows.push_back(new TimeWindow(next_day_timestamp + time_range.first, next_day_timestamp + time_range.second));
        }
    }
    // 处理最后一天
    if (day_diff == 0)
    {
        return intersection_time_windows;
    }
    std::vector<std::pair<int, int>> last_day_timestamp_range = this->work_calendar->get_wday_time(late_date.tm_wday);
    for (auto &time_range : last_day_timestamp_range)
    {
        long start_time = late_date_timestamp + time_range.first;
        long end_time = late_date_timestamp + time_range.second;
        if (start_time > late_timestamp)
        {
            continue;
        }
        intersection_time_windows.push_back(new TimeWindow(start_time, std::min(end_time, late_timestamp)));
    }

    return intersection_time_windows;
}
