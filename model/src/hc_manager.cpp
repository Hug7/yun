/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#include "hc_manager.h"

// ====== implement of Load HardConstraintManager ======
HardConstraintManager::~HardConstraintManager()
{
    for (auto &constraint : this->constrs)
    {
        delete constraint;
    }
}

void HardConstraintManager::add_constr(HardConstraint *constr)
{
    this->constrs.push_back(constr);
    ++this->len;

    // sort the constraints in the order of their priority
    std::sort(this->constrs.begin(), this->constrs.end(),
              [](const HardConstraint *a, const HardConstraint *b)
              {
                  return a->priority > b->priority;
              });
}

void HardConstraintManager::eval_constrs(Load *load, LoadConstrProfile::UPtr &constr_profile)
{    
    for (auto &constr : this->constrs)
    {
        auto hard_score = constr->eval(load);
        constr_profile->infesible |= hard_score->is_infeasible();
        constr_profile->total_hard_penalty += hard_score->get_score();
        constr_profile->hard_constr_scores.push_back(std::move(hard_score));
    }
}
