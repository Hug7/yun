/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#include "sc_manager.h"

#include <format>
#include <magic_enum/magic_enum.hpp>

#include "sc_dist.h"

// ====== implement of SoftConstraintManager ======
SoftConstraintManager::SoftConstraintManager(const Parameter* parameter) {
  // 可配置软约束
  for (const auto& soft_constr_item : parameter->soft_constr_items) {
    switch (soft_constr_item.code) {
      case SoftConstrCode::Distance:
        this->add_constr(SoftConstrParser::distance(soft_constr_item));
        break;
      default:
        throw std::invalid_argument(std::format("unknown soft constraint code={}",
                                                magic_enum::enum_name(soft_constr_item.code)));
        break;
    }
  }
}

SoftConstraintManager::~SoftConstraintManager() {
  for (const auto& constraint : this->constrs) {
    delete constraint;
  }
}

void SoftConstraintManager::add_constr(SoftConstraint* constr) {
  this->constrs.push_back(constr);
  ++this->len;

  // sort the constraints in the order of their priority
  std::ranges::sort(this->constrs, [](const SoftConstraint* a, const SoftConstraint* b) {
    return a->priority > b->priority;
  });
}

void SoftConstraintManager::eval_constrs(Load* load,
                                         const LoadConstrProfile::UPtr& constr_profile) const {
  for (const auto& constr : this->constrs) {
    auto soft_score = constr->eval(load);
    if (soft_score->value > 0) {
      constr_profile->total_soft_penalty += soft_score->get_score();
      constr_profile->soft_constr_scores.push_back(std::move(soft_score));
    }
  }
}

// ====== implement of SoftConstrParser ======
ScDistance* SoftConstrParser::distance(const SoftConstrItem& soft_constr_item) {
  const double dist_factor = soft_constr_item.params.parse_double("dist_factor");
  return new ScDistance(dist_factor);
}
