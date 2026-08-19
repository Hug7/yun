/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#include "cc_manager.h"

// ====== implement of Load CostConstraintManager ======
CostConstraintManager::~CostConstraintManager() {
  for (auto& constr : this->constrs) {
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

void CostConstraintManager::eval_constrs(Load* load, LoadConstrProfile::UPtr& constr_profile) {
  for (auto& constr : this->constrs) {
    auto cost_socre = constr->eval(load);
    constr_profile->infesible |= cost_socre->is_infeasible();
    constr_profile->total_cost += cost_socre->get_score();
    constr_profile->cost_constr_scores.push_back(std::move(cost_socre));
  }
}
