/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <yaml-cpp/yaml.h>

#include <memory>
#include <string>
#include <unordered_set>

#include "bdm_label.h"
#include "bdm_time_window.h"
#include "c_constant.h"
#include "c_yaml_utils.h"
#include "pdm_parameter_constraint.h"
#include "se_scenario.h"

/**
 * @brief 计划时间范围
 * @details
 */
class PlanDatetimeRange {
 public:
  /**
   * @brief 开始时间
   * @details 格式为 "yyyy-mm-dd hh:mm" 的时间戳(s)
   */
  long start_time{TimeWindowParameter::DEFAULT_PLAN_DATETIME_RANGE_START};
  /**
   * @brief 结束时间
   * @details 格式为 "yyyy-mm-dd hh:mm" 的时间戳(s)
   */
  long end_time{TimeWindowParameter::DEFAULT_PLAN_DATETIME_RANGE_END};

  PlanDatetimeRange() = default;

  void set_start_datetime(const std::string& start_datetime);

  void set_end_datetime(const std::string& end_datetime);

  /**
   * @brief 创建默认的时间窗口
   * @details 根据计划时间范围创建一个时间窗口
   */
  [[nodiscard]] TimeWindow* create_default_time_window() const;
};

/**
 * @brief cargo order分组规则
 */
class CargoOrderGroupRule {
 public:
  /**
   * @brief 订单的labelset
   */
  const Labelset* order_labelset;
  /**
   * @brief group的参考label
   */
  Label* label;
  /**
   * @brief label在labelset的索引
   */
  int label_ind4labelset;
  /**
   * @brief 执行group动作的参考分组
   * @details 为空时按照`label value`种类分组，不在分组内的`label value`的cargo order也按照`label
   * value`种类分组
   */
  std::vector<std::unordered_set<int>> label_value_ind_groups;

  explicit CargoOrderGroupRule(const Labelset* order_labelset)
      : order_labelset{order_labelset},
        label(nullptr),
        label_ind4labelset(-1),
        label_value_ind_groups() {};

  /**
   * @brief 设置分组规则
   * @param label_code 标签编码
   * @param label_value_groups
   */
  void set_rule(std::string label_code,
                const std::vector<std::vector<std::string>>& label_value_groups);
};

/**
 * @brief parameter configuration
 */
class Parameter {
 public:
  /**
   * @brief logger
   */
  const std::shared_ptr<spdlog::logger> logger;
  /**
   * @brief 计划时间范围
   */
  PlanDatetimeRange* plan_datetime_range{nullptr};
  /**
   * @brief 策略文件名
   */
  std::string strategy_file_name{"Strategy.lua"};
  /**
   * @brief 提货和卸货站点的模式
   */
  PickDropPatternType pick_drop_pattern{PickDropPatternType::SPMD};
  /**
   * @brief 装货和卸货顺序的规则
   */
  LoadUnloadPolicyType load_unload_policy{LoadUnloadPolicyType::FILO};

  // order
  /**
   * @brief cargo order分组规则
   */
  CargoOrderGroupRule* cargo_order_group_rule{nullptr};

  // constraints
  /**
   * @brief 硬约束条目列表
   */
  std::vector<HardConstrItem> hard_constr_items{};
  /**
   * @brief 硬约束条目列表
   */
  std::vector<SoftConstrItem> soft_constr_items{};

  explicit Parameter(const std::shared_ptr<spdlog::logger>& logger) : logger{logger} {};

  ~Parameter();

  void parse_plan_datetime_range(const YamlNode& param_node);

  void parse_strategy_file_name(const YamlNode& param_node);

  void parse_pick_drop_pattern(const YamlNode& param_node);

  void parse_load_unload_policy(const YamlNode& param_node);

  void parse_cargo_order_group_rule(const YamlNode& param_node, const Labelset* order_labelset);

  void parse_hard_constraints(const YamlNode& param_node);

  void parse_soft_constraints(const YamlNode& param_node);
  /**
   * @brief 后处理
   * @details 根据场景数据更新参数
   * @param scenario 场景数据
   */
  void post_process(const Scenario* scenario);
};

/**
 * @brief 读取参数
 * @param root_dir 根文件夹
 * @param scenario 场景
 * @param logger logger
 */
Parameter* load_parameter(const std::string& root_dir, const Scenario* scenario,
                          const std::shared_ptr<spdlog::logger>& logger);
