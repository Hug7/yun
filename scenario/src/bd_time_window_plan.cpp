/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#include "bd_time_window_plan.h"
#include <vector>

#include "c_constant.h"

// ====== implement of TimeWindowPlan ======
TimeWindowPlan::TimeWindowPlan(const TimeWindow* time_window, int work_time) {
  this->early_arr = time_window->early;
  this->late_arr = time_window->late;
  this->early_dest = this->early_arr + work_time;
  this->late_dest = this->late_arr + work_time;
  this->wait_time = 0;
  this->over_time = 0;
}

TimeWindowPlan::UPtr TimeWindowPlan::deep_copy() {
  return std::make_unique<TimeWindowPlan>(this->early_arr, this->late_arr, this->early_dest,
                                          this->late_dest, this->wait_time, this->over_time);
}

bool TimeWindowPlan::is_zero_wait_over_time() {
  return this->wait_time <= 0 && this->over_time <= 0;
}

bool TimeWindowPlan::is_zero_wait_time() { return this->wait_time <= 0; }

bool TimeWindowPlan::is_zero_over_time() { return this->over_time <= 0; }

TimeWindowPlan::UPtr TimeWindowPlanFactory::default_time_window_plan() {
  return std::make_unique<TimeWindowPlan>(
      TimeWindowParameter::DEFAULT_EARLY_TIME, TimeWindowParameter::DEFAULT_LATE_TIME,
      TimeWindowParameter::DEFAULT_EARLY_TIME, TimeWindowParameter::DEFAULT_LATE_TIME, 0, 0);
}

TimeWindowPlan::VecUPtr TimeWindowPlanFactory::default_time_window_plans() {
  auto res = std::vector<TimeWindowPlan::UPtr>(1);
  res[0] = default_time_window_plan();
  return res;
}
