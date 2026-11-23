/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#include "prob_problem.h"

#include <memory>
#include <stdexcept>

#include "prob_policy.h"

// ====== implement of Problem ======
Problem::Problem(const Scenario* scenario, Parameter* parameter)
    : scenario(scenario), parameter(parameter) {
  this->lu_policy = LoadUnloadPolicyFactory::create_policy(parameter->load_unload_policy);
  switch (this->parameter->pick_drop_pattern) {
    case PickDropPatternType::SPMD:
      this->pd_pattern = new PatternSPMD();
      break;
    case PickDropPatternType::MPMD:
      throw std::invalid_argument("unsupported pick drop pattern!");
      break;
    default:
      throw std::invalid_argument("invalid pick drop pattern!");
      break;
  }

  this->load_context = new LoadContext(this->scenario, this->parameter);

  this->hc_manager = new HardConstraintManager(scenario, parameter);
  this->sc_manager = new SoftConstraintManager(parameter);
  this->cc_manager = new CostConstraintManager(scenario);
}

Problem::~Problem() {
  delete this->lu_policy;
  delete this->pd_pattern;
  delete this->load_context;
  delete this->hc_manager;
  delete this->sc_manager;
  delete this->cc_manager;
}

void Problem::eval_load(Load* load) const {
  if (load->constr_profile == nullptr) {
    load->constr_profile = std::make_unique<LoadConstrProfile>();
  }
  this->eval_into(load, load->constr_profile);
}

LoadConstrProfile::UPtr Problem::evaluate(Load* load) const {
  auto constr_profile = std::make_unique<LoadConstrProfile>();
  this->eval_into(load, constr_profile);
  return constr_profile;
}

void Problem::eval_into(Load* load, const LoadConstrProfile::UPtr& constr_profile) const {
  constr_profile->reset();

  this->hc_manager->eval_constrs(load, constr_profile);

  this->cc_manager->eval_constrs(load, constr_profile);

  this->sc_manager->eval_constrs(load, constr_profile);

  // update obj val
  constr_profile->update_obj_val();
}
