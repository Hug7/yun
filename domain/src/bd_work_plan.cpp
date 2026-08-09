/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#include "c_time_utils.h"
#include "c_chrono_util.h"
#include "bd_work_plan.h"

// =============== Calendar ===============
Calendar::Calendar(CalendarTimeRangeType time_range_type)
{
    this->time_range_type = time_range_type;
    if (this->time_range_type == CalendarTimeRangeType::WEEKLY)
    {
        this->day_time_ranges = std::vector<std::vector<std::pair<int, int>>>(7, std::vector<std::pair<int, int>>());
    }
    else if (this->time_range_type == CalendarTimeRangeType::DAILY)
    {
        this->day_time_ranges = std::vector<std::vector<std::pair<int, int>>>(1, std::vector<std::pair<int, int>>());
    }
    else
    {
        throw std::runtime_error("exist unsupported calendar time range type");
    }
}

void Calendar::set_ond_day_time_ranges(const int day_ind, std::vector<std::pair<int, int>> &time_ranges)
{
    if (!time_ranges.empty())
    {
        this->day_time_ranges[day_ind] = time_ranges;
    }
    else
    {
        std::vector<std::pair<int, int>> default_time_ranges;
        default_time_ranges.emplace_back(0, 86400);
        this->day_time_ranges[day_ind] = default_time_ranges;
    }
}

std::vector<TimeWindow *> Calendar::intersection(TimeWindow *tw)
{
    std::vector<TimeWindow *> res_time_windows;
    auto early_dt = chrono_util::from_unix_date(tw->early);
    auto late_dt = chrono_util::from_unix_date(tw->late);
    if (this->time_range_type == CalendarTimeRangeType::WEEKLY)
    {
        while (early_dt <= late_dt)
        {
            int day_of_week = chrono_util::day_of_week(early_dt);
            for (auto &time_range : this->day_time_ranges[day_of_week])
            {
                long cur_early = chrono_util::to_unix(early_dt) + time_range.first;
                long cur_late = chrono_util::to_unix(early_dt) + time_range.second;
                cur_early = std::max(cur_early, tw->early);
                cur_late = std::min(cur_late, tw->late);
                if (cur_early >= cur_late)
                {
                    continue;
                }
                if (res_time_windows.size() > 0) {
                    auto last_tw = res_time_windows.back();
                    if ((cur_early - last_tw->late) <= TimeWindowParameter::INTERVAL_SECS_CONTINUOUS_TIME_BUCKET)
                    {
                        res_time_windows.pop_back();
                        res_time_windows.push_back(new TimeWindow(last_tw->early, cur_late));
                        delete last_tw;
                    } else {
                        res_time_windows.push_back(new TimeWindow(cur_early, cur_late));
                    }
                } else {
                    res_time_windows.push_back(new TimeWindow(cur_early, cur_late));
                }
            }
            early_dt = chrono_util::add_days(early_dt, 1);
        }
    }
    else if (this->time_range_type == CalendarTimeRangeType::DAILY)
    {
        while (early_dt <= late_dt)
        {
            for (auto &time_range : this->day_time_ranges[0])
            {
                long cur_early = chrono_util::to_unix(early_dt) + time_range.first;
                long cur_late = chrono_util::to_unix(early_dt) + time_range.second;
                cur_early = std::max(cur_early, tw->early);
                cur_late = std::min(cur_late, tw->late);
                if (cur_early >= cur_late)
                {
                    continue;
                }
                if (res_time_windows.size() > 0) {
                    auto last_tw = res_time_windows.back();
                    if ((cur_early - last_tw->late) <= TimeWindowParameter::INTERVAL_SECS_CONTINUOUS_TIME_BUCKET)
                    {
                        res_time_windows.pop_back();
                        res_time_windows.push_back(new TimeWindow(last_tw->early, cur_late));
                        delete last_tw;
                    } else {
                        res_time_windows.push_back(new TimeWindow(cur_early, cur_late));
                    }
                } else {
                    res_time_windows.push_back(new TimeWindow(cur_early, cur_late));
                }
            }
            early_dt = chrono_util::add_days(early_dt, 1);
        }
    }

    return res_time_windows;
}

// =============== Calendar ===============

WorkPlan::WorkPlan()
{
    this->fixed_pick_time = 0;
    this->fixed_drop_time = 0;
    this->pick_calendar = nullptr;
    this->drop_calendar = nullptr;
    this->restrict_calendar = nullptr;
    this->work_effect = nullptr;
}

WorkPlan::~WorkPlan()
{
    delete this->pick_calendar;
    delete this->drop_calendar;
    delete this->restrict_calendar;
    delete this->work_effect;
}

void WorkPlan::set_fixed_drop_time(int fixed_drop_time)
{
    this->fixed_drop_time = fixed_drop_time;
}

void WorkPlan::set_fixed_pick_time(int fixed_pick_time)
{
    this->fixed_pick_time = fixed_pick_time;
}

void WorkPlan::set_work_effect(WorkEffect *work_effect)
{
    this->work_effect = work_effect;
}

void WorkPlan::set_calendar(Calendar *calendar, CalendarType calendar_type)
{
    if (calendar_type == CalendarType::PICK)
    {
        this->pick_calendar = calendar;
    }
    else if (calendar_type == CalendarType::DROP)
    {
        this->drop_calendar = calendar;
    }
    else if (calendar_type == CalendarType::RESTRICT)
    {
        this->restrict_calendar = calendar;
    }
    else
    {
        throw std::runtime_error("exist unsupported calendar type");
    }
}