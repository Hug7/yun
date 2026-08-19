/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <string>

#include "dm_load.h"
#include "dm_score.h"

/**
 * @brief the attributes of hard constraint
 */
class HardConstraint {
 public:
  /**
   * @brief unique encoding with hard constraints
   */
  const std::string code;
  /**
   * @brief priority of the constraint
   * @details priority=0 is lower priority
   */
  const int priority;
  /**
   * @brief is the constraint related to the sequence
   */
  const bool seq_type;
  /**
   * @brief is the constraint related to the vehicle
   */
  const bool vehicle_type;

  HardConstraint(const std::string& code, const int priority, const bool seq_type,
                 const bool vehicle_type)
      : code(code), priority(priority), seq_type(seq_type), vehicle_type(vehicle_type) {};

  virtual ~HardConstraint() = default;

  /**
   * @brief calculate the score of the hard constraint
   */
  virtual HardConstrScore::UPtr eval(Load* load) = 0;
};
