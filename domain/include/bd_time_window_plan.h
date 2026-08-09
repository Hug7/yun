/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "bd_time_window.h"

class TimeWindowPlan
{
public:
    /**
     * early arrive time
     */
    long early_arr;
    /**
     * late arrive time
     */
    long late_arr;
    /**
     * early destination time
     */
    long early_dest;
    /**
     * late destination time
     */
    long late_dest;
    /**
     * wait time
     */
    long wait_time;
    /**
     * over time
     */
    long over_time;

    TimeWindowPlan() : early_arr(0), late_arr(0), early_dest(0), late_dest(0), wait_time(0), over_time(0) {};

    TimeWindowPlan(long early_arr, long late_arr,
                   long early_dest, long late_dest,
                   long wait_time, long over_time) : early_arr(early_arr),
                                                     late_arr(late_arr),
                                                     early_dest(early_dest),
                                                     late_dest(late_dest),
                                                     wait_time(wait_time),
                                                     over_time(over_time) {};

    TimeWindowPlan(TimeWindow *time_window, int work_time);

    TimeWindowPlan *deep_copy();

    bool is_zero_wait_over_time();

    bool is_zero_wait_time();

    bool is_zero_over_time();
};
