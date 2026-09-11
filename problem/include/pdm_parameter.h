/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <string>
#include <unordered_set>

#include "bdm_label.h"
#include "bdm_time_window.h"
#include "c_constant.h"
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
      : order_labelset{order_labelset}, label(nullptr), label_ind4labelset(-1), label_value_ind_groups() {};

  /**
   * @brief 设置分组规则
   * @param label_code 标签编码
   * @param label_value_code_groups
   */
  void set_rule(std::string label_code,
                const std::vector<std::vector<std::string>>& label_value_code_groups);
};

/**
 * @brief parameter configuration
 */
class Parameter {
 public:
  /**
   * @brief 计划时间范围
   */
  PlanDatetimeRange* plan_datetime_range;
  /**
   * @brief 策略文件名
   */
  std::string strategy_file_name{"strategy.lua"};
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
  CargoOrderGroupRule* cargo_order_group_rule;

  // constraints
  /**
   * @brief hard constraint: load中的最大提货节点数
   */
  int max_pick_node_count{HardConstraintParameter::DEFAULT_MAX_PICK_NODE_COUNT};
  /**
   * @brief hard constraint: load中的最大卸货节点数
   */
  int max_drop_node_count{HardConstraintParameter::DEFAULT_MAX_DROP_NODE_COUNT};
  /**
   * @brief hard constraint: 时间窗约束开关
   */
  bool time_window_constr_enabled{true};
  /**
   * @brief soft constraint: "ScDist" default distance factor
   */
  double sc_constr_dist_factor{SoftConstraintParameter::SC_DIST_DEFAULT_DIST_FACTOR};
  /**
   * @brief cost constraint: "CcDist" default distance factor
   */
  double cc_constr_dist_factor{CostConstraintParameter::CC_DIST_DEFAULT_DIST_FACTOR};

  explicit Parameter(const Labelset* order_labelset);

  ~Parameter();

  /**
   * @brief 后处理
   * @details 根据场景数据更新参数
   * @param scenario 场景数据
   */
  void post_process(const Scenario* scenario);

  /**
   * @brief 设置策略文件名
   * @param file_name 策略文件名
   */
  void set_strategy_file_name(const std::string& file_name);
};
