/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <string>

#include "c_constant.h"
#include "bdm_time_window.h"
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
  long start_time{TimeWindowParameter::DEFAULT_PLAN_DATETIME_RANGE};
  /**
   * @brief 结束时间
   * @details 格式为 "yyyy-mm-dd hh:mm" 的时间戳(s)
   */
  long end_time{TimeWindowParameter::DEFAULT_PLAN_DATETIME_RANGE};

  PlanDatetimeRange() {};

  void set_start_datetime(const std::string& start_datetime);

  void set_end_datetime(const std::string& end_datetime);

  /**
   * @brief 创建默认的时间窗口
   * @details 根据计划时间范围创建一个时间窗口
   */
  TimeWindow* create_default_time_window();
};

/**
 * @brief parameter configuration
 */
class Parameter {
 public:
  /**
   * @brief 计划时间范围
   */
  PlanDatetimeRange *plan_datetime_range;
  /**
   * @brief 提货和卸货站点的模式
   */
  PickDropPatternType pick_drop_pattern{PickDropPatternType::SPMD};
  /**
   * @brief 装货和卸货顺序的规则
   */
  LoadUnloadPolicyType load_unload_policy{LoadUnloadPolicyType::FILO};

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

  Parameter();

  ~Parameter();

  /**
   * @brief 后处理
   * @details 根据场景数据更新参数
   * @param 场景数据
   */
  void post_process(const Scenario* scenario);
};
