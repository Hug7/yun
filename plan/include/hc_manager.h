/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <vector>

#include "dm_load.h"
#include "hc_constraint.h"

/**
 * @brief hard constraint manager
 */
class HardConstraintManager {
 public:
  std::vector<HardConstraint*> constrs;

  int len;

  HardConstraintManager() : constrs(), len(0) {};

  ~HardConstraintManager();

  /**
   * @brief add a hard constraint to the manager
   */
  void add_constr(HardConstraint* constr);

  /**
   * @brief evaluate all hard constraints
   */
  void eval_constrs(Load* load, LoadConstrProfile::UPtr& constr_profile);
};
