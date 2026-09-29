/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#include "cc_manager.h"

// ====== implement of Load CostConstraintManager ======
CostConstraintManager::~CostConstraintManager() {
  for (const auto& constr : this->constrs) {
    delete constr;
  }
}

void CostConstraintManager::add_constr(CostConstraint* constr) {
  this->constrs.push_back(constr);
  ++this->len;

  // sort the constraints in the order of their priority
  std::sort(
      this->constrs.begin(), this->constrs.end(),
      [](const CostConstraint* a, const CostConstraint* b) { return a->priority > b->priority; });
}

void CostConstraintManager::eval_constrs(Load* load,
                                         const LoadConstrProfile::UPtr& constr_profile) const {
  for (const auto& constr : this->constrs) {
    auto cost_score = constr->eval(load);
    constr_profile->infeasible |= cost_score->is_infeasible();
    constr_profile->total_cost += cost_score->get_score();
    constr_profile->cost_constr_scores.push_back(std::move(cost_score));
  }
}
