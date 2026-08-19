/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#include "s_solver.h"

#include <memory>

#include "pl_plan.h"

// ====== implement of Load Solver ======
void Solver::solve() {
  // precheck
  
  // create a new plan
  Plan *plan = new Plan(this->scenario);
  // distrubute vehicle resources and order resources
  
}
