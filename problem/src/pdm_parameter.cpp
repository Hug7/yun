/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#include "pdm_parameter.h"

#include "bdm_time_window.h"
#include "c_time_utils.h"

// ====== implement of PlanDateTimeRange ======
void PlanDatetimeRange::set_start_datetime(const std::string& start_datetime) {
  this->start_time = TimeParse::parse_tm_to_sec(start_datetime, TimeParse::fmt_yyyymmddhhmm_1);
}

void PlanDatetimeRange::set_end_datetime(const std::string& end_datetime) {
  this->end_time = TimeParse::parse_tm_to_sec(end_datetime, TimeParse::fmt_yyyymmddhhmm_1);
}

TimeWindow* PlanDatetimeRange::create_default_time_window() {
  return new TimeWindow(this->start_time, this->end_time);
}

// ====== implement of Parameter ======
Parameter::Parameter() { this->plan_datetime_range = new PlanDatetimeRange(); }

void Parameter::post_process(const Scenario* scenario) {
  // 根据订单时间范围，更新计划时间范围
  // --找到订单时间范围
  long min_pick_time = std::numeric_limits<long>::max();
  long max_drop_time = std::numeric_limits<long>::min();
  for (const auto& order : scenario->cargo_order_manager->cargo_orders) {
    min_pick_time = std::min(min_pick_time, order->pick_time_window->early);
    max_drop_time = std::max(max_drop_time, order->drop_time_window->late);
  }
  // --更新计划时间范围
  if (this->plan_datetime_range->start_time < min_pick_time) {
    this->plan_datetime_range->start_time = min_pick_time;
  }
  if (this->plan_datetime_range->end_time > max_drop_time) {
    this->plan_datetime_range->end_time = max_drop_time;
  }
}

Parameter::~Parameter() { delete this->plan_datetime_range; }
