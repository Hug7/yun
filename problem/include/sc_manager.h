/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <vector>

#include "dm_load.h"
#include "sc_constraint.h"

/**
 * @brief hard constraint manager
 */
class SoftConstraintManager {
 public:
  std::vector<SoftConstraint*> constrs;

  int len;

  SoftConstraintManager() : constrs(), len(0) {};

  ~SoftConstraintManager();

  /**
   * @brief add a soft constraint to the manager
   */
  void add_constr(SoftConstraint* constr);

  /**
   * @brief evaluate the all soft constraints
   */
  void eval_constrs(Load* load, LoadConstrProfile::UPtr& constr_profile);
};
