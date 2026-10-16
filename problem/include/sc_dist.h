/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "sc_constraint.h"

/**
 * @brief 软约束-行驶距离
 */
class ScDistance : public SoftConstraint {
 public:
  /**
   * @brief 距离系数
   */
  const double dist_factor;

  explicit ScDistance(const double dist_factor)
      : SoftConstraint(SoftConstrCode::Distance, 0, true, false), dist_factor(dist_factor) {}

  SoftConstrScore::UPtr eval(Load* load) override;
};
