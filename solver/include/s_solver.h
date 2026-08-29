/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <string>

#include "se_scenario.h"
#include "pdm_parameter.h"
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

  /**
   * @brief 预校验
   * @details 校验是否存在不满足约束的订单，如果存在则提前退出
   */
  bool precheck();

  /**
   * @brief 求解
   */
  void solve();
};
