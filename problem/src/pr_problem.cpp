/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#include "pr_problem.h"

#include <memory>

// ====== implement of Problem ======
Problem::Problem(const Scenario* scenario, Parameter* parameter)
    : scenario(scenario), parameter(parameter) {
  switch (this->parameter->pick_drop_pattern) {
    case PickDropPatternType::SPMD:
      this->pd_pattern = new SPMD(scenario);
      break;
    case PickDropPatternType::MPMD:
      throw std::invalid_argument("unsupported pick drop pattern!");
      break;
    default:
      throw std::invalid_argument("invalid pick drop pattern!");
      break;
  }

  this->hc_manager = new HardConstraintManager();
  this->sc_manager = new SoftConstraintManager();
  this->cc_manager = new CostConstraintManager();
}

Problem::~Problem() { delete this->parameter; }

void Problem::eval_load(Load* load) {
  // todo 是否需要重置待讨论
  LoadConstrProfile::UPtr& constr_profile = load->constr_profile;
  constr_profile->reset();

  this->hc_manager->eval_constrs(load, constr_profile);

  this->cc_manager->eval_constrs(load, constr_profile);

  this->sc_manager->eval_constrs(load, constr_profile);
  // update obj val
  constr_profile->update_obj_val();
}

LoadConstrProfile::UPtr Problem::tmp_eval_load(Load* load) {
  LoadConstrProfile::UPtr constr_profile = std::make_unique<LoadConstrProfile>();
  this->hc_manager->eval_constrs(load, constr_profile);
  if (constr_profile->infesible) {
    return constr_profile;
  }

  this->cc_manager->eval_constrs(load, constr_profile);
  if (constr_profile->infesible) {
    return constr_profile;
  }

  this->sc_manager->eval_constrs(load, constr_profile);

  // update obj val
  constr_profile->update_obj_val();

  return constr_profile;
}

InfeasibleCargoOrder::UPtr Problem::check_feasibility(const CargoOrder* cargo_order) {
  // construct order
  auto dim_vals = scenario->dimension_manager->empty_dim_values();
  auto labelset_value = scenario->label_manager->order_labelset->empty_labelset_value();
  auto labelset_value_bitset =
      scenario->label_manager->order_labelset->empty_labelset_value_bitset();
  auto cargo_orders = std::vector<const CargoOrder*>({
      cargo_order,
  });
  auto available_vehicle_bitset = scenario->carrier_manager->full_vehicle_bitset();

  auto order = new Order(this->parameter->plan_time_range, cargo_orders, dim_vals, labelset_value,
                         std::move(labelset_value_bitset), std::move(available_vehicle_bitset));
  auto orders = std::vector<Order*>({order});

  auto infeasible_cargo_order = std::make_unique<InfeasibleCargoOrder>(cargo_orders);

  // try all vehicles
  const auto& vehicles = scenario->carrier_manager->vehicles;
  bool has_feasible_flag = false;
  for (auto& vehicle : vehicles) {
    // check vehicle resource
    if (vehicle->unusable()) {
      infeasible_cargo_order->record_vehicle_infeasible_reasons(
          InfeasibleReasonCollection::VEHICLE_REOURCE, vehicle);
      continue;
    }
    auto cur_load = this->construct_load_by_order(orders, vehicle);
    // check feasibility
    if (cur_load->constr_profile->is_infesible()) {
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

  delete order;

  if (has_feasible_flag) {
    return nullptr;
  }

  return infeasible_cargo_order;
}
