/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "dm_resource.h"
#include "pr_problem.h"
#include "se_scenario.h"

class Plan {
 public:
  const Scenario* scenario;

  const VehicleResource* vehicle_resource;

  const OrderResource* order_resource;

  Problem* problem;

  Plan(const Scenario* scenario) : scenario(scenario) {}
};
