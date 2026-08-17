/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#include "sc_manager.h"

// ====== implement of Load SoftConstraintManager ======
SoftConstraintManager::~SoftConstraintManager() {
  for (auto& constraint : this->constrs) {
    delete constraint;
  }
}

void SoftConstraintManager::add_constr(SoftConstraint* constr) {
  this->constrs.push_back(constr);
  ++this->len;

  // sort the constraints in the order of their priority
  std::sort(
      this->constrs.begin(), this->constrs.end(),
      [](const SoftConstraint* a, const SoftConstraint* b) { return a->priority > b->priority; });
}

void SoftConstraintManager::eval_constrs(Load* load, LoadConstrProfile::UPtr& constr_profile) {
  for (auto& constr : this->constrs) {
    auto soft_score = constr->eval(load);
    if (soft_score->value > 0) {
      constr_profile->total_soft_penalty += soft_score->get_score();
      constr_profile->soft_constr_scores.push_back(std::move(soft_score));
    }
  }
}
