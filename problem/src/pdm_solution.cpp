/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#include "pdm_solution.h"

#include <ranges>

// ====== implement of Solution ======
double Solution::total_cost() const {
  // 统计目标值(hard score + soft score + cost score)
  double total_obj_val = 0;
  for (const auto load : this->loads) {
    total_obj_val += load->constr_profile->obj_val;
  }
  // 未指派的订单(固定惩罚)
  for (const auto& order : this->unassigned_orders | std::views::values) {
    total_obj_val += static_cast<int>(order->cargo_orders.size()) * 1000;
  }

  return total_obj_val;
}

void Solution::add_load(Load* load) {
  // todo 临时校验
  if (load == nullptr) {
    throw std::invalid_argument("attempting to add an null load in the solution space");
  }
  if (load->is_infeasible()) {
    throw std::invalid_argument("attempting to add an infeasible load in the solution space");
  }
  // 添加load
  this->loads.push_back(load);
  // 占用车辆资源
  this->vehicle_resource->occupy(load->vehicle);
}

void Solution::add_unassigned_order(const Order* order) {
  this->unassigned_orders.emplace(order->ind, order);
}

void Solution::remove_unassigned_order(const Order* order) {
  this->unassigned_orders.erase(order->ind);
}

void Solution::remove_unassigned_order_by_key(const int order_ind) {
  this->unassigned_orders.erase(order_ind);
}
