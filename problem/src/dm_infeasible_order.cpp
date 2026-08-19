/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#include "dm_infeasible_order.h"

// ====== implement of InfeasibleCargoOrder ======
void InfeasibleCargoOrder::record_common_infeasible_reasons(const InfeasibleReason &reason) {
  this->common_infeasible_reasons.try_emplace(reason.code, reason);
}

void InfeasibleCargoOrder::record_vehicle_infeasible_reasons(const InfeasibleReason &reason, Vehicle* vehicle) {
    auto it = this->vehicle_infeasible_reasons.find(vehicle);
    if (it == this->vehicle_infeasible_reasons.end()) {
      this->vehicle_infeasible_reasons[vehicle].try_emplace(reason.code, reason);
    } else {
      it->second.try_emplace(reason.code, reason);
    }
}

void InfeasibleCargoOrder::record_hard_score(HardConstrScore::UPtr& hard_score, Vehicle* vehicle) {
  if (hard_score->is_feasible()) {
    return;
  }
  if (!hard_score->is_vehicle()) {
    this->record_common_infeasible_reasons(hard_score->reason);
  } else {
    this->record_vehicle_infeasible_reasons(hard_score->reason, vehicle);
  }
}

void InfeasibleCargoOrder::record_cost_score(CostConstrScore::UPtr& cost_score, Vehicle* vehicle) {
  if (cost_score->is_feasible()) {
    return;
  }
  if (!cost_score->is_vehicle()) {
    this->record_common_infeasible_reasons(cost_score->reason);
  } else {
    this->record_vehicle_infeasible_reasons(cost_score->reason, vehicle);
  }
}
