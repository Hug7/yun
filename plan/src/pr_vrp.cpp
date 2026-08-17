/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#include "pr_vrp.h"

// ====== implement of ProblemVRP ======
Load* ProblemVRP::construct_load_by_order(std::vector<Order*>& orders) {
  Load* load = this->pd_pattern->create_load(this->scenario);
  for (auto& order : orders) {
    this->pd_pattern->add_order(load, order);
  }
  // try using different vehicle
  const std::vector<Vehicle*>& vehicles = this->scenario->carrier_manager->vehicles;
  LoadConstrProfile::UPtr best_profile = nullptr;
  Vehicle* best_vehilce = nullptr;
  for (auto& vehicle : vehicles) {
    // change vehicle
    load->change_vehicle(vehicle);
    // temporary evaluate of load
    auto cur_constr_profile = this->tmp_eval_load(load);
    if (cur_constr_profile->is_infesible()) {
      continue;
    }
    if (best_vehilce == nullptr) {
      best_profile = std::move(cur_constr_profile);
      best_vehilce = vehicle;
    } else if (cur_constr_profile->dominate(best_profile)) {
      best_profile = std::move(cur_constr_profile);
      best_vehilce = vehicle;
    }
  }
  // change vehicle
  load->change_vehicle(best_vehilce);
  this->eval_load(load);

  return load;
}
