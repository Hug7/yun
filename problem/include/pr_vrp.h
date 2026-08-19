/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <vector>

#include "dm_load.h"
#include "dm_order.h"
#include "pr_problem.h"
#include "se_scenario.h"

class ProblemVRP : public Problem {
 public:
  ProblemVRP(Scenario* scenario) : Problem(scenario) {};

  Load* construct_load_by_order(std::vector<Order*>& orders) override;

  Load* construct_load_by_order(std::vector<Order*>& orders, Vehicle* vehicle) override;
};
