/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "bdm_cargo_order.h"
#include "bdm_vehicle.h"
#include "pdm_infeasible.h"
#include "pdm_order.h"
#include "pdm_score.h"

/**
 * @brief 不可解的订单
 */
class InfeasibleCargoOrder {
 public:
  using UPtr = std::unique_ptr<InfeasibleCargoOrder>;
  using VecUPtr = std::vector<UPtr>;

  std::vector<CargoOrder*> cargo_orders;

  std::unordered_map<std::string, InfeasibleReason> common_infeasible_reasons;

  std::unordered_map<Vehicle*, std::unordered_map<std::string, InfeasibleReason>>
      vehicle_infeasible_reasons;

  explicit InfeasibleCargoOrder(const std::vector<CargoOrder*>& cargo_orders)
      : cargo_orders(cargo_orders), common_infeasible_reasons(), vehicle_infeasible_reasons() {}

  void record_common_infeasible_reasons(const InfeasibleReason& reason);

  void record_vehicle_infeasible_reasons(const InfeasibleReason& reason, Vehicle* vehicle);

  void record_hard_score(HardConstrScore::UPtr& hard_score, Vehicle* vehicle);

  void record_cost_score(CostConstrScore::UPtr& cost_score, Vehicle* vehicle);
};

class InfeasibleOrder {
 public:
  using UPtr = std::unique_ptr<InfeasibleOrder>;

  Order* cargo_order;

  std::unordered_map<std::string, InfeasibleReason> common_infeasible_reasons;

  std::unordered_map<Vehicle*, std::vector<InfeasibleReason>> vehicle_infeasible_reasons;

  explicit InfeasibleOrder(Order* cargo_order)
      : cargo_order(cargo_order), common_infeasible_reasons(), vehicle_infeasible_reasons() {}
};
