/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#include "pdm_parameter.h"

#include <format>
#include <unordered_set>

#include "bdm_time_window.h"
#include "c_time_utils.h"

// ====== implement of PlanDateTimeRange ======
void PlanDatetimeRange::set_start_datetime(const std::string& start_datetime) {
  this->start_time = TimeParse::parse_tm_to_sec(start_datetime, TimeParse::fmt_yyyymmddhhmm_1);
}

void PlanDatetimeRange::set_end_datetime(const std::string& end_datetime) {
  this->end_time = TimeParse::parse_tm_to_sec(end_datetime, TimeParse::fmt_yyyymmddhhmm_1);
}

TimeWindow* PlanDatetimeRange::create_default_time_window() const {
  return new TimeWindow(this->start_time, this->end_time);
}

// ====== implement of CargoOrderGroupRule ======
void CargoOrderGroupRule::set_rule(
    std::string label_code, const std::vector<std::vector<std::string>>& label_value_code_groups) {
  this->label = this->order_labelset->get_label(label_code);
  this->label_ind4labelset = this->order_labelset->get_label_ind(label_code);
  // 校验 label code 是否存在
  if (this->label == nullptr) {
    throw std::runtime_error(
        std::format("CargoOrderGroupRule: label code {} not exists in order labelset", label_code));
  }
  if (label_value_code_groups.empty()) {
    return;
  }
  // 校验 label value 是否存在
  std::unordered_set<std::string> label_value_set;  // 标签值的集合
  for (const auto& label_value_codes : label_value_code_groups) {
    std::unordered_set<int> label_value_indices;
    for (const auto& label_value_code : label_value_codes) {
      // 校验是否存在重复的标签值
      if (label_value_set.contains(label_value_code)) {
        throw std::runtime_error(std::format(
            "CargoOrderGroupRule: label value code {} is repetitive", label_value_code));
      }
      label_value_set.insert(label_value_code);
      LabelValue* label_value = this->label->get_label_value(label_value_code);
      if (label_value == nullptr) {
        throw std::runtime_error(
            std::format("CargoOrderGroupRule: label code {} not contains label value code {}",
                        label_code, label_value_code));
      }
      label_value_indices.insert(label_value->ind);
    }
    this->label_value_ind_groups.push_back(label_value_indices);
  }
}

// ====== implement of Parameter ======
Parameter::Parameter(const Labelset* order_labelset) {
  this->plan_datetime_range = new PlanDatetimeRange();
  this->cargo_order_group_rule = new CargoOrderGroupRule(order_labelset);
}

Parameter::~Parameter() {
  delete this->plan_datetime_range;
  delete this->cargo_order_group_rule;
}

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

void Parameter::set_strategy_file_name(const std::string& file_name) {
  this->strategy_file_name = file_name;
};
