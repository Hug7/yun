/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <string>

#include "dm_load.h"
#include "dm_score.h"

/**
 * @brief the attributes of cost constraint
 */
class CostConstraint {
 public:
  /**
   * @brief unique encoding with cost constraints
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

  CostConstraint(const std::string code, const int priority, const bool seq_type,
                 const bool vehicle_type)
      : code(code), priority(priority), seq_type(seq_type), vehicle_type(vehicle_type) {};

  virtual ~CostConstraint() = default;

  /**
   * @brief evaluate the score of the cost constraint
   */
  virtual CostConstrScore::UPtr eval(Load* load) = 0;
};
