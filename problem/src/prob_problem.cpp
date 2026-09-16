/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#include "prob_problem.h"

#include <memory>

#include "pdm_order.h"
#include "prob_policy.h"

// ====== implement of Problem ======
Problem::Problem(const Scenario* scenario, Parameter* parameter)
    : scenario(scenario), parameter(parameter) {
  this->lu_policy = LoadUnloadPolicyFactory::create_policy(parameter->load_unload_policy);
  switch (this->parameter->pick_drop_pattern) {
    case PickDropPatternType::SPMD:
      this->pd_pattern = new PatternSPMD();
      break;
    case PickDropPatternType::MPMD:
      throw std::invalid_argument("unsupported pick drop pattern!");
      break;
    default:
      throw std::invalid_argument("invalid pick drop pattern!");
      break;
  }

  this->load_context = new LoadContext(this->scenario, this->parameter->plan_datetime_range);

  this->hc_manager = new HardConstraintManager();
  this->sc_manager = new SoftConstraintManager();
  this->cc_manager = new CostConstraintManager();
}

Problem::~Problem() {}

void Problem::eval_load(Load* load) const {
  // todo 是否需要重置待讨论
  LoadConstrProfile::UPtr& constr_profile = load->constr_profile;
  constr_profile->reset();

  this->hc_manager->eval_constrs(load, constr_profile);

  this->cc_manager->eval_constrs(load, constr_profile);

  this->sc_manager->eval_constrs(load, constr_profile);
  // update obj val
  constr_profile->update_obj_val();
}

LoadConstrProfile::UPtr Problem::tmp_eval_load(Load* load) const {
  LoadConstrProfile::UPtr constr_profile = std::make_unique<LoadConstrProfile>();
  this->hc_manager->eval_constrs(load, constr_profile);
  if (constr_profile->infeasible) {
    return constr_profile;
  }

  this->cc_manager->eval_constrs(load, constr_profile);
  if (constr_profile->infeasible) {
    return constr_profile;
  }

  this->sc_manager->eval_constrs(load, constr_profile);

  // update obj val
  constr_profile->update_obj_val();

  return constr_profile;
}

InfeasibleCargoOrder::UPtr Problem::check_feasibility(CargoOrder* cargo_order) const {
  // construct order
  auto cargo_orders = std::vector<CargoOrder*>({cargo_order});
  const Order* order =
      OrderFactory::creat_tmp_order(cargo_orders, parameter->plan_datetime_range, scenario);
  auto orders = std::vector<const Order*>({order});

  auto infeasible_cargo_order = std::make_unique<InfeasibleCargoOrder>(cargo_orders);

  // try all vehicles
  const auto& vehicles = scenario->carrier_manager->vehicles;
  bool has_feasible_flag = false;
  for (auto& vehicle : vehicles) {
    // check vehicle resource
    if (vehicle->unusable()) {
      infeasible_cargo_order->record_vehicle_infeasible_reasons(
          InfeasibleReasonCollection::VEHICLE_RESOURCE, vehicle);
      continue;
    }
    auto cur_load = this->construct_load_by_order(orders, vehicle);
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
    delete cur_load;
    if (has_feasible_flag) {
      break;
    }
  }
  // release order
  delete order;

  if (has_feasible_flag) {
    // 有可行解，则返回空指针
    return nullptr;
  }

  return infeasible_cargo_order;
}

Load* Problem::construct_load_by_order(std::vector<const Order*>& orders) const {
  Load* load = this->pd_pattern->create_load(this->load_context);
  for (const auto order : orders) {
    this->pd_pattern->insert_last_drop(load, order);
  }
  // try using different vehicle
  const std::vector<Vehicle*>& vehicles = this->scenario->carrier_manager->vehicles;
  LoadConstrProfile::UPtr best_profile = nullptr;
  Vehicle* best_vehicle = nullptr;
  for (const auto vehicle : vehicles) {
    // check vehicle resource
    if (vehicle->unusable()) {
      continue;
    }
    // change vehicle
    load->change_vehicle(vehicle);
    // temporary evaluate of load
    auto cur_constr_profile = this->tmp_eval_load(load);
    if (cur_constr_profile->is_infeasible()) {
      continue;
    }
    if (best_vehicle == nullptr) {
      best_profile = std::move(cur_constr_profile);
      best_vehicle = vehicle;
    } else if (cur_constr_profile->dominate(best_profile)) {
      best_profile = std::move(cur_constr_profile);
      best_vehicle = vehicle;
    }
  }
  // 没有任何可行车辆时不能切换车辆，直接标记不可行交由调用方处理
  if (best_vehicle == nullptr) {
    load->constr_profile->set_infeasible();
    return load;
  }
  // change vehicle
  load->change_vehicle(best_vehicle);
  this->eval_load(load);

  return load;
}

Load* Problem::construct_load_by_order(std::vector<const Order*>& orders, Vehicle* vehicle) const {
  Load* load = this->pd_pattern->create_load(this->load_context);
  for (const auto order : orders) {
    this->pd_pattern->insert_last_drop(load, order);
  }
  load->change_vehicle(vehicle);
  this->eval_load(load);

  return load;
}
