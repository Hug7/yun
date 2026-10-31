/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#include "pdm_parameter.h"

#include <format>
#include <magic_enum/magic_enum.hpp>
#include <unordered_set>

#include "bdm_time_window.h"
#include "c_time_utils.h"
#include "c_yaml_utils.h"

namespace {
constexpr char PARAMETER_FILE_NAME[] = "Parameter.yaml";  // 参数文件名
constexpr char KEY_START_TIME[] = "start_time";
constexpr char KEY_END_TIME[] = "end_time";
constexpr char KEY_STRATEGY_FILE[] = "strategy_file";
constexpr char KEY_PICK_DROP_PATTERN[] = "pick_drop_pattern";
constexpr char KEY_LOAD_UNLOAD_POLICY[] = "load_unload_policy";
constexpr char KEY_CARGO_ORDER_GROUP_RULE[] = "cargo_order_group_rule";
constexpr char KEY_CARGO_ORDER_GROUP_RULE_LABEL_CODE[] = "label_code";
constexpr char KEY_CARGO_ORDER_GROUP_RULE_LABEL_VALUE_GROUP[] = "label_value_group";
constexpr char KEY_HARD_CONSTRAINTS[] = "hard_constraints";
constexpr char KEY_SOFT_CONSTRAINTS[] = "soft_constraints";

[[noreturn]] void fail(const std::string& path, const std::string& reason,
                       const std::shared_ptr<spdlog::logger>& logger) {
  logger->error(std::format("{} {}: {}", PARAMETER_FILE_NAME, path, reason));
  throw std::runtime_error(std::format("{} {}: {}", PARAMETER_FILE_NAME, path, reason));
}

}  // namespace

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
    std::string label_code, const std::vector<std::vector<std::string>>& label_value_groups) {
  this->label = this->order_labelset->get_label(label_code);
  this->label_ind4labelset = this->order_labelset->get_label_ind(label_code);
  // 校验 label code 是否存在
  if (this->label == nullptr) {
    throw std::runtime_error(
        std::format("CargoOrderGroupRule: label code {} not exists in order labelset", label_code));
  }
  if (label_value_groups.empty()) {
    return;
  }
  // 校验 label value 是否存在
  std::unordered_set<std::string> label_value_set;  // 标签值的集合
  for (const auto& label_value_codes : label_value_groups) {
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
Parameter::~Parameter() {
  delete this->plan_datetime_range;
  delete this->cargo_order_group_rule;
}

void Parameter::parse_plan_datetime_range(const YamlNode& param_node) {
  this->plan_datetime_range = new PlanDatetimeRange();
  if (param_node.contains_key(KEY_START_TIME)) {
    this->plan_datetime_range->set_start_datetime(param_node.parse_string(KEY_START_TIME));
  }
  if (param_node.contains_key(KEY_END_TIME)) {
    this->plan_datetime_range->set_end_datetime(param_node.parse_string(KEY_END_TIME));
  }
}

void Parameter::parse_strategy_file_name(const YamlNode& param_node) {
  if (param_node.contains_key(KEY_STRATEGY_FILE)) {
    this->strategy_file_name = param_node.parse_string(KEY_STRATEGY_FILE);
  }
}

void Parameter::parse_pick_drop_pattern(const YamlNode& param_node) {
  if (param_node.contains_key(KEY_PICK_DROP_PATTERN)) {
    const auto name = param_node.parse_string(KEY_PICK_DROP_PATTERN);
    const auto pattern = magic_enum::enum_cast<PickDropPatternType>(name);
    if (!pattern.has_value()) {
      fail(KEY_PICK_DROP_PATTERN, std::format("未知提卸模式: {}", name), this->logger);
    }
    this->pick_drop_pattern = *pattern;
  }
}

void Parameter::parse_load_unload_policy(const YamlNode& param_node) {
  if (param_node.contains_key(KEY_LOAD_UNLOAD_POLICY)) {
    const auto name = param_node.parse_string(KEY_LOAD_UNLOAD_POLICY);
    const auto policy = magic_enum::enum_cast<LoadUnloadPolicyType>(name);
    if (!policy.has_value()) {
      fail(KEY_LOAD_UNLOAD_POLICY, std::format("未知装卸策略: {}", name), this->logger);
    }
    this->load_unload_policy = *policy;
  }
}

void Parameter::parse_cargo_order_group_rule(const YamlNode& param_node,
                                             const Labelset* order_labelset) {
  this->cargo_order_group_rule = new CargoOrderGroupRule(order_labelset);
  if (param_node.contains_key(KEY_CARGO_ORDER_GROUP_RULE)) {
    const auto cargo_order_group_rule_yaml_node = param_node.parse_map(KEY_CARGO_ORDER_GROUP_RULE);
    const auto label_code =
        cargo_order_group_rule_yaml_node.parse_string(KEY_CARGO_ORDER_GROUP_RULE_LABEL_CODE);
    const auto label_value_group =
        cargo_order_group_rule_yaml_node.get_node(KEY_CARGO_ORDER_GROUP_RULE_LABEL_VALUE_GROUP)
            .as<std::vector<std::vector<std::string>>>();
    this->cargo_order_group_rule->set_rule(label_code, label_value_group);
  }
}

void Parameter::parse_hard_constraints(const YamlNode& param_node) {
  if (!param_node.contains_key(KEY_HARD_CONSTRAINTS)) {
    return;
  }
  const auto hard_constr_nodes = param_node.parse_list(KEY_HARD_CONSTRAINTS);
  this->hard_constr_items = parse_hard_constr_items(hard_constr_nodes);
}

void Parameter::parse_soft_constraints(const YamlNode& param_node) {
  if (!param_node.contains_key(KEY_SOFT_CONSTRAINTS)) {
    return;
  }
  const auto soft_constr_nodes = param_node.parse_list(KEY_SOFT_CONSTRAINTS);
  this->soft_constr_items = parse_soft_constr_items(soft_constr_nodes);
}

void Parameter::post_process(const Scenario* scenario) {}

Parameter* load_parameter(const std::string& root_dir, const Scenario* scenario,
                          const std::shared_ptr<spdlog::logger>& logger) {
  // 实例化参数
  Parameter* parameter = new Parameter(logger);
  // 读取参数yaml文件
  auto param_node = read_yaml(root_dir + "/" + PARAMETER_FILE_NAME, logger);
  // 解析参数
  parameter->parse_plan_datetime_range(param_node);
  parameter->parse_strategy_file_name(param_node);
  parameter->parse_pick_drop_pattern(param_node);
  parameter->parse_load_unload_policy(param_node);
  parameter->parse_cargo_order_group_rule(param_node, scenario->label_manager->order_labelset);
  // 解析-硬约束项
  parameter->parse_hard_constraints(param_node);
  // 解析-软约束项
  parameter->parse_soft_constraints(param_node);
  // 参数后处理
  parameter->post_process(scenario);

  return parameter;
}
