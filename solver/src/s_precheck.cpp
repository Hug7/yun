/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#include "s_precheck.h"

#include <spdlog/spdlog.h>

#include <memory>
#include <vector>

#include "bdm_cargo_order.h"
#include "bdm_vehicle.h"
#include "pdm_cache.h"
#include "pdm_order.h"
#include "se_scenario.h"

// ====== implement of SolverPrecheck ======
InfeasibleCargoOrder::UPtr SolverPrecheck::check_cargo_order(CargoOrder* cargo_order) const {
  Problem* problem = this->context->problem;

  // construct order
  auto cargo_orders = std::vector<CargoOrder*>({cargo_order});
  const std::unique_ptr<Order> order(
      OrderFactory::creat_tmp_order(cargo_orders, problem->parameter->plan_datetime_range,
                                    problem->scenario, &problem->tw_pattern_registry));
  auto orders = std::vector<const Order*>({order.get()});

  // 预检用临时 Order 构造 Load 并求值, 该 Order 在本函数末尾释放。这里仍挂一个函数局部的
  // 丢弃式写缓冲, 只为避免把预检产生的一次性条目带进正式求解。
  CacheStoreBuffer precheck_tw_buffer;

  auto infeasible_cargo_order = std::make_unique<InfeasibleCargoOrder>(cargo_orders);

  // try all vehicles
  const auto& vehicles = problem->scenario->carrier_manager->vehicles;
  bool has_feasible_flag = false;
  for (auto& vehicle : vehicles) {
    // check vehicle resource
    if (vehicle->unusable()) {
      infeasible_cargo_order->record_vehicle_infeasible_reasons(
          InfeasibleReasonCollection::VEHICLE_RESOURCE, vehicle);
      continue;
    }
    std::unique_ptr<Load> cur_load(
        this->context->load_editor->build(orders, vehicle, &precheck_tw_buffer));
    // check feasibility
    if (cur_load->constr_profile->is_infeasible()) {
      auto& cur_constr_profile = cur_load->constr_profile;
      // record hard constr scores
      for (auto& hard_score : cur_constr_profile->hard_constr_scores) {
        infeasible_cargo_order->record_hard_score(hard_score, vehicle);
      }
      // record cost constr scores
      for (auto& cost_score : cur_constr_profile->cost_constr_scores) {
        infeasible_cargo_order->record_cost_score(cost_score, vehicle);
      }
    } else {
      has_feasible_flag = true;
    }
    cur_load.reset();
    if (has_feasible_flag) {
      break;
    }
  }
  if (has_feasible_flag) {
    // 有可行解，则返回空指针
    return nullptr;
  }

  return infeasible_cargo_order;
}

bool SolverPrecheck::call() {
  this->infeasible_cargo_orders = std::vector<InfeasibleCargoOrder::UPtr>();
  for (const auto& cargo_order : this->context->scenario->cargo_order_manager->cargo_orders) {
    auto infeasible_cargo_order = this->check_cargo_order(cargo_order);
    if (infeasible_cargo_order == nullptr) {
      continue;
    }
    this->infeasible_cargo_orders.push_back(std::move(infeasible_cargo_order));
  }

  if (!this->infeasible_cargo_orders.empty()) {
    this->context->logger->warn("number of cargo orders {} are infeasible!",
                                this->infeasible_cargo_orders.size());
    // 将不可解的订单和原因记录到csv文件中
    this->context->visual_manager->visual_infeasible_cargo_order_to_json(
        this->infeasible_cargo_orders);
  }

  return this->infeasible_cargo_orders.empty();
}
