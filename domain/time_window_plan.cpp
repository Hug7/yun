#include "time_window_plan.h"

TimeWindowPlan::TimeWindowPlan()
{
    this->early_arr = 0;
    this->late_arr = 0;
    this->early_dest = 0;
    this->late_dest = 0;
    this->wait_time = 0;
    this->over_time = 0;
}

TimeWindowPlan::TimeWindowPlan(long early_arr, long late_arr, long early_dest, long late_dest, long wait_time, long over_time)
{
    this->early_arr = early_arr;
    this->late_arr = late_arr;
    this->early_dest = early_dest;
    this->late_dest = late_dest;
    this->wait_time = wait_time;
    this->over_time = over_time;
}

TimeWindowPlan::TimeWindowPlan(TimeWindow *time_window, int work_time)
{
    this->early_arr = time_window->early;
    this->late_arr = time_window->late;
    this->early_dest = this->early_arr + work_time;
    this->late_dest = this->late_arr + work_time;
    this->wait_time = 0;
    this->over_time = 0;
}

TimeWindowPlan::TimeWindowPlan(std::unique_ptr<TimeWindowPlan> &time_window)
{
    this->early_arr = time_window->early_arr;
    this->late_arr = time_window->late_arr;
    this->early_dest = time_window->late_arr;
    this->late_dest = time_window->late_dest;
    this->wait_time = time_window->wait_time;
    this->over_time = time_window->over_time;
}

std::unique_ptr<TimeWindowPlan> TimeWindowPlan::deep_copy()
{
    std::unique_ptr<TimeWindowPlan> time_window = std::make_unique<TimeWindowPlan>();
    time_window->early_arr = this->early_arr;
    time_window->late_arr = this->late_arr;
    time_window->early_dest = this->early_dest;
    time_window->late_dest = this->late_dest;
    time_window->wait_time = this->wait_time;
    time_window->over_time = this->over_time;

    return std::move(time_window);
}

bool TimeWindowPlan::is_zero_wait_over_time()
{
    return this->wait_time <= 0 && this->over_time <= 0;
}

bool TimeWindowPlan::is_zero_wait_time()
{
    return this->wait_time <= 0;
}

bool TimeWindowPlan::is_zero_over_time()
{
    return this->over_time <= 0;
}
