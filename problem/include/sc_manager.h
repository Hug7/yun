/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <vector>

#include "pdm_load.h"
#include "pdm_parameter.h"
#include "sc_constraint.h"
#include "sc_dist.h"
#include "se_scenario.h"

/**
 * @brief 软约束管理器
 */
class SoftConstraintManager {
 public:
  /**
   * @brief 软约束列表
   * @details 拥有指针的归属权
   */
  std::vector<SoftConstraint*> constrs;
  /**
   * @brief 软约束列表长度
   */
  int len{0};

  explicit SoftConstraintManager(const Parameter* parameter);

  ~SoftConstraintManager();

  /**
   * @brief 添加软约束到管理器中
   */
  void add_constr(SoftConstraint* constr);

  /**
   * @brief 评价所有软约束
   */
  void eval_constrs(Load* load, const LoadConstrProfile::UPtr& constr_profile) const;
};

namespace SoftConstrParser {

ScDistance* distance(const SoftConstrItem& soft_constr_item);

}  // namespace SoftConstrParser
