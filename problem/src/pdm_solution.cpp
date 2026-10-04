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
  this->add_load(load, load->get_orders());
}

void Solution::add_load(Load* load, const std::vector<const Order*>& orders) {
  // todo 临时校验
  if (load == nullptr) {
    throw std::invalid_argument("attempting to add an null load in the solution space");
  }
  if (load->is_infeasible()) {
    throw std::invalid_argument("attempting to add an infeasible load in the solution space");
  }
  // 添加 load
  this->loads.push_back(load);
  // 占用车辆资源
  this->vehicle_resource->occupy(load->vehicle);
  // 移除 unassigned orders
  for (const auto& order : orders) {
    this->unassigned_orders.erase(order->ind);
  }
}

void Solution::update_load(const int load_ind, Load* load) {
  // 释放 load_ind 上的 load 占用的资源
  this->release_vehicle_resource(this->loads[load_ind]);
  // 释放 load_ind 上的 load
  delete this->loads[load_ind];
  // 占用资源
  this->occupy_vehicle_resource(load);
  // 更新 load_ind 上的 load
  this->loads[load_ind] = load;
}

void Solution::remove_load_by_ind(const int load_ind) {
  // order_indices为空则代表移除整个车次
  const auto cur_load = this->loads[load_ind];
  // 将 cur_load 放到未指派的订单池中
  this->add_unassigned_orders(cur_load->get_orders());
  // 释放 cur_load 占用的车辆资源
  this->vehicle_resource->release(cur_load->vehicle);
  // 释放 cur_load
  delete cur_load;
  // 移除 loads 在索引 load_ind 的元素
  this->loads.erase(this->loads.begin() + load_ind);
}

void Solution::add_unassigned_order(const Order* order) {
  this->unassigned_orders.emplace(order->ind, order);
}

void Solution::add_unassigned_orders(const std::vector<const Order*>& orders) {
  for (const auto& order : orders) {
    this->unassigned_orders.emplace(order->ind, order);
  }
}

void Solution::add_unassigned_orders_by_activities(const std::vector<Activity*>& activities) {
  for (const auto& activity : activities) {
    this->unassigned_orders.emplace(activity->order->ind, activity->order);
  }
}

void Solution::remove_unassigned_order(const Order* order) {
  this->unassigned_orders.erase(order->ind);
}

void Solution::remove_unassigned_order_by_segment(const Segment* segment) {
  for (const auto& order : segment->orders) {
    this->unassigned_orders.erase(order->ind);
  }
}

void Solution::remove_unassigned_order_by_key(const int order_ind) {
  this->unassigned_orders.erase(order_ind);
}

void Solution::occupy_vehicle_resource(const Load* load) const {
  this->vehicle_resource->occupy(load->vehicle);
}

void Solution::release_vehicle_resource(const Load* load) const {
  this->vehicle_resource->release(load->vehicle);
}
