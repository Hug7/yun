/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <unordered_map>
#include <vector>

#include "pdm_load.h"
#include "pdm_order.h"
#include "pdm_resource.h"

class Solution {
 public:
  /**
   * @brief 车辆资源
   */
  VehicleResource::UPtr vehicle_resource;
  /**
   * @brief 未指派的订单集合
   */
  std::unordered_map<int, const Order*> unassigned_orders;
  /**
   * @brief 车次集合
   */
  std::vector<Load*> loads;

  Solution() = default;

  [[nodiscard]] double total_cost() const;

  void add_load(Load* load);

  void add_unassigned_order(const Order* order);

  void remove_unassigned_order(const Order* order);

  void remove_unassigned_order_by_key(int order_ind);
};
