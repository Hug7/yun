/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "hc_constraint_code.h"
#include "pdm_load.h"
#include "pdm_score.h"

/**
 * @brief 硬约束父类
 */
class HardConstraint {
 public:
  /**
   * @brief 硬约束编码
   */
  const HardConstrCode code;
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

  HardConstraint(const HardConstrCode code, const int priority, const bool seq_type,
                 const bool vehicle_type)
      : code(code), priority(priority), seq_type(seq_type), vehicle_type(vehicle_type) {};

  virtual ~HardConstraint() = default;

  /**
   * @brief 评价硬约束
   */
  virtual HardConstrScore::UPtr eval(Load* load) = 0;
};
