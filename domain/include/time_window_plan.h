#ifndef TIME_WINDOW_PLAN_H
#define TIME_WINDOW_PLAN_H

#include <memory>

#include "time_window.h"

struct TimeWindowPlan
{
    long early_arr;  // early arrive time
    long late_arr;   // late arrive time
    long early_dest; // early destination time
    long late_dest;  // late destination time
    long wait_time;  // wait time
    long over_time;  // over time

    TimeWindowPlan();
    TimeWindowPlan(long early_arr, long late_arr, long early_dest, long late_dest, long wait_time, long over_time);
    TimeWindowPlan(TimeWindow *time_window, int work_time);
    TimeWindowPlan(std::unique_ptr<TimeWindowPlan> &time_window);
    std::unique_ptr<TimeWindowPlan> deep_copy();
    bool is_zero_wait_over_time();
    bool is_zero_wait_time();
    bool is_zero_over_time();
};

#endif // TIME_WINDOW_PLAN_H

