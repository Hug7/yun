/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <unordered_map>
#include <vector>

#include "pdm_load.h"
#include "pdm_order.h"
#include "pdm_segment.h"
#include "pdm_resource.h"

class Solution {
 public:
  /**
   * @brief 车辆资源
   */
  VehicleResource::UPtr vehicle_resource;
  /**
   * @brief 订单视图集合
   */
  std::unordered_map<int, const Order*> orders_view;
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

  void add_load(Load* load, const std::vector<const Order*>& orders);

  void update_load(int load_ind, Load* load);

  void remove_load_by_ind(int load_ind);

  void add_unassigned_order(const Order* order);

  void add_unassigned_orders(const std::vector<const Order*>& orders);

  void add_unassigned_orders_by_activities(const std::vector<Activity*>& activities);

  void remove_unassigned_order(const Order* order);

  void remove_unassigned_order_by_segment(const Segment* segment);

  void remove_unassigned_order_by_key(int order_ind);

  void occupy_vehicle_resource(const Load* load) const;

  void release_vehicle_resource(const Load* load) const;
};
