/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <string>
#include <utility>

#include "cc_constraint_code.h"
#include "pdm_load.h"
#include "pdm_score.h"

/**
 * @brief the attributes of cost constraint
 */
class CostConstraint {
 public:
  /**
   * @brief 成本约束编码
   */
  CostConstrCode code;
  /**
   * @brief 约束的执行优先级
   * @details priority=0时，代表最后执行
   */
  const int priority;
  /**
   * @brief 是否为受顺序影响的约束
   */
  const bool seq_type;
  /**
   * @brief 是否为受车辆影响的约束
   */
  const bool vehicle_type;

  CostConstraint(CostConstrCode code, const int priority, const bool seq_type,
                 const bool vehicle_type)
      : code(code), priority(priority), seq_type(seq_type), vehicle_type(vehicle_type) {};

  virtual ~CostConstraint() = default;

  /**
   * @brief 评价成本约束
   */
  virtual CostConstrScore::UPtr eval(Load* load) = 0;
};
