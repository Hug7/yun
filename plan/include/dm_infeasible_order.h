/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "bd_cargo_order.h"
#include "bd_vehicle.h"
#include "dm_infeasible.h"
#include "dm_order.h"

class InfeasibleCargoOrder {
 public:
  using UPtr = std::unique_ptr<InfeasibleCargoOrder>;

  std::vector<const CargoOrder*> cargo_orders;

  std::unordered_map<std::string, InfeasibleReason> common_infeasible_reasons;

  std::unordered_map<Vehicle*, std::vector<InfeasibleReason>> vehicle_infeasible_reasons;

  InfeasibleCargoOrder(std::vector<const CargoOrder*>& cargo_orders)
      : cargo_orders(cargo_orders), common_infeasible_reasons(), vehicle_infeasible_reasons() {}
};

class InfeasibleOrder {
 public:
  using UPtr = std::unique_ptr<InfeasibleOrder>;

  Order* cargo_order;

  std::unordered_map<std::string, InfeasibleReason> common_infeasible_reasons;

  std::unordered_map<Vehicle*, std::vector<InfeasibleReason>> vehicle_infeasible_reasons;

  InfeasibleOrder(Order* cargo_order)
      : cargo_order(cargo_order), common_infeasible_reasons(), vehicle_infeasible_reasons() {}
};
