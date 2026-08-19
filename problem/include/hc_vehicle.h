/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <string>

#include "hc_constraint.h"

/**
 * @brief hard constraint for vehicle capacity
 * @details all orders are subject to first-pick priority post-drop
 */
class HcVehicleCapacity : public HardConstraint {
 public:
  HcVehicleCapacity() : HardConstraint("HcVehicleCapacity", 0, false, true) {};

  /**
   * @brief calculate the score of the hard constraint
   */
  HardConstrScore::UPtr eval(Load* load) override;
};
