/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "pdm_load.h"
#include "pdm_score.h"
#include "sc_constraint_code.h"

/**
 * @brief 软约束的父类
 */
class SoftConstraint {
 public:
  /**
   * @brief 软约束的编码
   */
  const SoftConstrCode code;
  /**
   * @brief 约束的执行优先级
   * @details priority=0时，代表最后执行
   */
  const int priority;
  /**
   * @brief 是否为受顺序影响的约束
   */
  const bool is_seq;
  /**
   * @brief 是否为受车辆影响的约束
   */
  const bool is_vehicle;

  SoftConstraint(const SoftConstrCode code, const int priority, const bool is_seq,
                 const bool is_vehicle)
      : code(code), priority(priority), is_seq(is_seq), is_vehicle(is_vehicle) {};

  virtual ~SoftConstraint() = default;

  /**
   * @brief 评价软约束
   */
  virtual SoftConstrScore::UPtr eval(Load* load) = 0;
};
