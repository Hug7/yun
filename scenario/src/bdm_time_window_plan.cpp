/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#include "bdm_time_window_plan.h"

#include <cstddef>
#include <vector>

#include "c_constant.h"

// ====== implement of TimeWindowPlan ======
TimeWindowPlan::TimeWindowPlan(const std::int64_t early_arr, const std::int64_t late_arr,
                               const std::int64_t early_depart, const std::int64_t late_depart,
                               const std::int64_t wait_time, const std::int64_t over_time)
    : early_arr(early_arr),
      late_arr(late_arr),
      early_depart(early_depart),
      late_depart(late_depart),
      wait_time(wait_time),
      over_time(over_time) {}

TimeWindowPlan::TimeWindowPlan(const TimeWindow& time_window, const std::int64_t work_time) {
  this->early_arr = time_window.early;
  this->late_arr = time_window.late;
  this->early_depart = this->early_arr + work_time;
  this->late_depart = this->late_arr + work_time;
  this->wait_time = 0;
  this->over_time = 0;
}

TimeWindowPlan::TimeWindowPlan(const TimeWindow& time_window) {
  this->early_arr = time_window.early;
  this->late_arr = time_window.late;
  this->early_depart = this->early_arr;
  this->late_depart = this->late_arr;
  this->wait_time = 0;
  this->over_time = 0;
}

bool TimeWindowPlan::is_zero_wait_over_time() const {
  return this->wait_time <= 0 && this->over_time <= 0;
}

bool TimeWindowPlan::is_zero_wait_time() const { return this->wait_time <= 0; }

bool TimeWindowPlan::is_zero_over_time() const { return this->over_time <= 0; }

TimeWindowPlan TimeWindowPlanFactory::default_time_window_plan() {
  return {TimeWindowParameter::DEFAULT_EARLY_TIME,
          TimeWindowParameter::DEFAULT_LATE_TIME,
          TimeWindowParameter::DEFAULT_EARLY_TIME,
          TimeWindowParameter::DEFAULT_LATE_TIME,
          0,
          0};
}

std::vector<TimeWindowPlan> TimeWindowPlanFactory::default_time_window_plans() {
  return {default_time_window_plan()};
}

std::vector<TimeWindowPlan> TimeWindowPlanFactory::create_time_window_plans(
    const std::vector<TimeWindow>& time_windows) {
  std::vector<TimeWindowPlan> res;
  res.reserve(time_windows.size());
  for (const auto& time_window : time_windows) {
    res.emplace_back(time_window);
  }
  return res;
}
