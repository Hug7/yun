/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#include "pdm_parameter.h"

#include <filesystem>
#include <format>
#include <magic_enum/magic_enum.hpp>
#include <unordered_set>

#include "bdm_time_window.h"
#include "c_time_utils.h"

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
constexpr char KEY_MAX_COUNTS[] = "max_counts";

[[noreturn]] void fail(const std::string& path, const std::string& reason,
                       const std::shared_ptr<spdlog::logger>& logger) {
  logger->error(std::format("{} {}: {}", PARAMETER_FILE_NAME, path, reason));
  throw std::runtime_error(std::format("{} {}: {}", PARAMETER_FILE_NAME, path, reason));
}

/**
 * @brief 读取yaml参数
 * @param root_dir 根目录
 * @param logger logger
 * @return yaml参数
 */
YAML::Node load_parameter_yaml(const std::string& root_dir,
                               const std::shared_ptr<spdlog::logger>& logger) {
  const std::string file = root_dir + "/" + PARAMETER_FILE_NAME;
  if (!std::filesystem::exists(file)) {
    logger->error("File {} does not exist.", PARAMETER_FILE_NAME);
    throw std::filesystem::filesystem_error(
        PARAMETER_FILE_NAME, std::error_code(static_cast<int>(std::errc::no_such_file_or_directory),
                                             std::generic_category()));
  }

  YAML::Node param_node;
  try {
    param_node = YAML::LoadFile(file);
  } catch (const YAML::Exception& e) {
    throw std::runtime_error(std::format("{} 解析失败: {}", PARAMETER_FILE_NAME, e.what()));
  }
  if (!param_node || !param_node.IsMap()) {
    fail("", "yaml文件根节点解析为空", logger);
  }

  return param_node;
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

void Parameter::parse_plan_datetime_range(const YAML::Node& param_node) {
  this->plan_datetime_range = new PlanDatetimeRange();
  if (param_node[KEY_START_TIME]) {
    this->plan_datetime_range->set_start_datetime(param_node[KEY_START_TIME].as<std::string>());
  }
  if (param_node[KEY_END_TIME]) {
    this->plan_datetime_range->set_end_datetime(param_node[KEY_END_TIME].as<std::string>());
  }
}

void Parameter::parse_strategy_file_name(const YAML::Node& param_node) {
  if (param_node[KEY_STRATEGY_FILE]) {
    this->strategy_file_name = param_node[KEY_STRATEGY_FILE].as<std::string>();
  }
}

void Parameter::parse_pick_drop_pattern(const YAML::Node& param_node) {
  if (param_node[KEY_PICK_DROP_PATTERN]) {
    const auto name = param_node[KEY_PICK_DROP_PATTERN].as<std::string>();
    const auto pattern = magic_enum::enum_cast<PickDropPatternType>(name);
    if (!pattern.has_value()) {
      fail(KEY_PICK_DROP_PATTERN, std::format("未知提卸模式: {}", name), this->logger);
    }
    this->pick_drop_pattern = *pattern;
  }
}

void Parameter::parse_load_unload_policy(const YAML::Node& param_node) {
  if (param_node[KEY_LOAD_UNLOAD_POLICY]) {
    const auto name = param_node[KEY_LOAD_UNLOAD_POLICY].as<std::string>();
    const auto policy = magic_enum::enum_cast<LoadUnloadPolicyType>(name);
    if (!policy.has_value()) {
      fail(KEY_LOAD_UNLOAD_POLICY, std::format("未知装卸策略: {}", name), this->logger);
    }
    this->load_unload_policy = *policy;
  }
}

void Parameter::parse_cargo_order_group_rule(const YAML::Node& param_node,
                                             Labelset* order_labelset) {
  this->cargo_order_group_rule = new CargoOrderGroupRule(order_labelset);
  if (param_node[KEY_CARGO_ORDER_GROUP_RULE]) {
    const auto label_code =
        param_node[KEY_CARGO_ORDER_GROUP_RULE][KEY_CARGO_ORDER_GROUP_RULE_LABEL_CODE]
            .as<std::string>();
    const auto label_value_group =
        param_node[KEY_CARGO_ORDER_GROUP_RULE][KEY_CARGO_ORDER_GROUP_RULE_LABEL_VALUE_GROUP]
            .as<std::vector<std::vector<std::string>>>();
    this->cargo_order_group_rule->set_rule(label_code, label_value_group);
  }
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

Parameter* load_parameter(const std::string& root_dir, const Scenario* scenario,
                          const std::shared_ptr<spdlog::logger>& logger) {
  // 实例化参数
  Parameter* parameter = new Parameter();
  // 读取参数yaml文件
  auto param_node = load_parameter_yaml(root_dir, logger);
  // 解析参数
  parameter->parse_plan_datetime_range(param_node);
  parameter->parse_strategy_file_name(param_node);
  parameter->parse_pick_drop_pattern(param_node);
  parameter->parse_load_unload_policy(param_node);
  parameter->parse_cargo_order_group_rule(param_node, scenario->label_manager->order_labelset);

  // 参数后处理
  parameter->post_process(scenario);

  return parameter;
}
