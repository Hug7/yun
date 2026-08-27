/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <string>

#include "se_scenario.h"
#include "dm_parameter.h"
#include "pr_problem.h"

class Solver {
 public:
  const std::string roo_dir;

  Scenario *scenario;

  Parameter *parameter;

  Problem *problem;

  std::vector<InfeasibleCargoOrder::UPtr> infeasible_cargo_orders;

  Solver(const std::string &roo_dir) : roo_dir(roo_dir) {};

  ~Solver();

  void load_scenario();

  void load_parameter();

  void create_problem();

  void precheck();

  void solve();
};
