/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#include "dm_solution.h"

double Solution::total_cost() {
  double total_cost = 0;
  for (auto load : this->loads) {
    total_cost += load->route_profile->total_dist;
  }

  return total_cost;
}
