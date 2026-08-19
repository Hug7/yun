/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <vector>

#include "dm_load.h"
#include "dm_resource.h"

class Solution {
 public:
  VehicleResourceUsage::UPtr resource;

  OrderResource* order_resource;

  std::vector<Load*> loads;

  Solution() {}

  double total_cost();
};
