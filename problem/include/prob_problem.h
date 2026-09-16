/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <vector>

#include "bdm_cargo_order.h"
#include "cc_manager.h"
#include "hc_manager.h"
#include "pdm_infeasible_order.h"
#include "pdm_load.h"
#include "pdm_order.h"
#include "pdm_parameter.h"
#include "prob_pattern.h"
#include "prob_policy.h"
#include "sc_manager.h"
#include "se_scenario.h"

class Problem {
 public:
  const Scenario* scenario;

  Parameter* parameter;

  LoadUnloadPolicy* lu_policy;

  PickDropPattern* pd_pattern;

  LoadContext* load_context;

  HardConstraintManager* hc_manager;

  SoftConstraintManager* sc_manager;

  CostConstraintManager* cc_manager;

  Problem(const Scenario* scenario, Parameter* parameter);

  virtual ~Problem();

  /**
   * @brief evaluate all constraints
   */
  virtual void eval_load(Load* load) const;

  virtual LoadConstrProfile::UPtr tmp_eval_load(Load* load) const;
  /**
   * @brief check cargo order feasibility
   */
  virtual InfeasibleCargoOrder::UPtr check_feasibility(CargoOrder* cargo_order) const;
  /**
   * @brief construct a new Load object for best vehicle
   */
  virtual Load* construct_load_by_order(std::vector<const Order*>& orders) const;
  /**
   * @brief construct a new Load object for a vehicle
   */
  virtual Load* construct_load_by_order(std::vector<const Order*>& orders, Vehicle* vehicle) const;
};
