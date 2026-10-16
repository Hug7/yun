/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <vector>

#include "hc_constraint.h"
#include "hc_label.h"
#include "hc_location.h"
#include "pdm_load.h"
#include "pdm_parameter.h"
#include "pdm_parameter_constraint.h"
#include "se_scenario.h"

/**
 * @brief 硬约束管理器
 */
class HardConstraintManager {
 public:
  /**
   * @brief 硬约束列表
   * @details 拥有指针的归属权
   */
  std::vector<HardConstraint*> constrs;
  /**
   * @brief 硬约束列表长度
   */
  int len{0};

  explicit HardConstraintManager(const Scenario* scenario, const Parameter* parameter);

  ~HardConstraintManager();

  /**
   * @brief 添加硬约束到管理器中
   */
  void add_constr(HardConstraint* constr);

  /**
   * @brief 评价所有硬约束
   */
  void eval_constrs(Load* load, const LoadConstrProfile::UPtr& constr_profile) const;
};

namespace HardConstrParser {

HcMaxPickNodeCount* max_pick_node_count(const HardConstrItem& hard_constr_item);

HcMaxDropNodeCount* max_drop_node_count(const HardConstrItem& hard_constr_item);

HcOrderLabelValueMaxCount* order_label_value_max_count(const Scenario* scenario,
                                                       const HardConstrItem& hard_constr_item);

}  // namespace HardConstrParser