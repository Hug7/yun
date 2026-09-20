/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#include "pdm_workspace.h"

#include <ranges>

// ====== implement of Workspace ======
Workspace::Workspace(const SolverContext* _context, const OrderPool* order_pool)
    : context(_context) {
  // this->unassigned_orders.reserve(order_pool->len);
  for (const auto order : order_pool->orders) {
    this->unassigned_orders.emplace(order->ind, order);
  }
  this->vehicle_resource =
      ResourceFactory::create_vehicle_resource(_context->scenario->carrier_manager);
}

Workspace::~Workspace() {
  for (const auto& load : this->loads) {
    delete load;
  }
  this->unassigned_orders.clear();
}

void Workspace::push(const std::vector<Load*>& _loads,
                     const std::unordered_map<int, const Order*>& _unassigned_orders) {
  this->load_order_stack.push(std::make_unique<LoadOrderBuffer>(_loads, _unassigned_orders));
}

void Workspace::pop() {
  if (this->load_order_stack.empty()) {
    throw std::runtime_error("temporary storage of workspace is empty!!");
  }
  const auto load_order = std::move(this->load_order_stack.top());
  this->load_order_stack.pop();
  this->loads.reserve(this->loads.size() + load_order->loads.size());
  this->loads.insert(this->loads.end(), load_order->loads.begin(), load_order->loads.end());

  if (!load_order->unassigned_orders.empty()) {
    if (this->unassigned_orders.empty()) {
      this->unassigned_orders = std::move(load_order->unassigned_orders);
    } else {
      this->unassigned_orders.merge(load_order->unassigned_orders);
    }
  }
}

Solution* Workspace::generate_sol() const {
  const auto sol = new Solution();
  // copy-车辆资源
  sol->vehicle_resource = this->vehicle_resource->deep_copy();
  // copy-load
  const auto pd_pattern = this->context->problem->pd_pattern;
  const size_t load_len = this->loads.size();
  sol->loads = std::vector<Load*>(load_len);
  for (size_t u = 0; u < load_len; ++u) {
    sol->loads[u] = pd_pattern->deep_copy_load(this->loads[u]);
  }
  // copy-未指派订单
  sol->unassigned_orders = this->unassigned_orders;
  return sol;
}

void Workspace::move_solution(Solution* sol) {
  this->vehicle_resource = std::move(sol->vehicle_resource);
  this->unassigned_orders.clear();
  this->unassigned_orders = std::move(sol->unassigned_orders);
  this->loads.clear();
  this->loads = std::move(sol->loads);

  delete sol;
}

std::vector<Load*> Workspace::loads_view() {
  std::vector<Load*> shadow_clone_loads;
  shadow_clone_loads.insert(shadow_clone_loads.end(), this->loads.begin(), this->loads.end());
  std::stack<LoadOrderBuffer::UPtr> tmp_load_order_stack;
  while (!this->load_order_stack.empty()) {
    auto tmp_load_order = std::move(this->load_order_stack.top());
    this->load_order_stack.pop();
    shadow_clone_loads.insert(shadow_clone_loads.end(), tmp_load_order->loads.begin(),
                              tmp_load_order->loads.end());

    tmp_load_order_stack.push(std::move(tmp_load_order));
  }
  this->load_order_stack = std::move(tmp_load_order_stack);

  return shadow_clone_loads;
}

std::vector<const Order*> Workspace::unassigned_orders_view() {
  std::vector<const Order*> shadow_clone_unassigned_orders;
  for (const auto& order : this->unassigned_orders | std::views::values) {
    shadow_clone_unassigned_orders.emplace_back(order);
  }
  std::stack<LoadOrderBuffer::UPtr> tmp_load_order_stack;
  while (!this->load_order_stack.empty()) {
    auto tmp_load_order = std::move(this->load_order_stack.top());
    this->load_order_stack.pop();
    for (const auto& order : tmp_load_order->unassigned_orders | std::views::values) {
      shadow_clone_unassigned_orders.emplace_back(order);
    }

    tmp_load_order_stack.push(std::move(tmp_load_order));
  }
  this->load_order_stack = std::move(tmp_load_order_stack);

  return shadow_clone_unassigned_orders;
}
