/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <vector>

#include "bd_cargo_order.h"
#include "cc_manager.h"
#include "dm_load.h"
#include "dm_order.h"
#include "dm_infeasible_order.h"
#include "hc_manager.h"
#include "pr_pattern.h"
#include "sc_manager.h"
#include "se_scenario.h"

class Problem {
 public:
  const Scenario* scenario;

  PickDropPattern* pd_pattern;

  HardConstraintManager* hc_manager;

  SoftConstraintManager* sc_manager;

  CostConstraintManager* cc_manager;

  Problem(const Scenario* scenario);

  /**
   * @brief evaluate all constraints
   */
  virtual void eval_load(Load* load);

  virtual LoadConstrProfile::UPtr tmp_eval_load(Load* load);
  /**
   * @brief check cargo order feasibility
   */
  virtual InfeasibleCargoOrder::UPtr check_feasibility(const CargoOrder* cargo_order);
  /**
   * @brief construct a new Load object for best vehicle
   */
  virtual Load* construct_load_by_order(std::vector<Order*>& orders) = 0;
  /**
   * @brief construct a new Load object for a vehicle
   */
  virtual Load* construct_load_by_order(std::vector<Order*>& orders, Vehicle* vehicle) = 0;
};
