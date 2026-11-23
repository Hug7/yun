/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "hc_constraint.h"

/**
 * @brief hard constraint for vehicle capacity
 * @details all orders are subject to first-pick priority post-drop
 */
class HcVehicleCapacity : public HardConstraint {
 public:
  HcVehicleCapacity() : HardConstraint(HardConstrCode::VehicleCapacity, 0, false, true) {};

  /**
   * @brief calculate the score of the hard constraint
   */
  HardConstrScore::UPtr eval(Load* load) override;

  /**
   * @brief 预检: 插入后峰值装不下任何可用车型时剪枝
   */
  [[nodiscard]] HcPrecheckVerdict precheck(const HcPrecheckInput& input) const override;
};
