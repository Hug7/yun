/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#include "pr_problem.h"

#include <memory>

#include "cc_dist.h"
#include "hc_location.h"
#include "hc_vehicle.h"

// ====== implement of Problem ======
Problem::Problem(const Scenario* scenario) : scenario(scenario) {
  switch (scenario->parameter->pick_drop_pattern) {
    case PickDropPatternType::SPMD:
      this->pd_pattern = new SPMD(scenario);
      break;
    case PickDropPatternType::MPMD:
      throw std::invalid_argument("unsupported pick drop pattern!");
      break;
    case PickDropPatternType::PDP:
      throw std::invalid_argument("unsupported pick drop pattern!");
      break;
    default:
      throw std::invalid_argument("invalid pick drop pattern!");
      break;
  }

  const Parameter* parameter = scenario->parameter;

  this->hc_manager = new HardConstraintManager();
  this->sc_manager = new SoftConstraintManager();
  this->cc_manager = new CostConstraintManager();

  // hard constraints: base
  this->hc_manager->add_constr(new HcVehicleCapacity());
  // todo add hard constraints by scenario

  // == hard constraints: max drop node count
  if (parameter->max_pick_node_count > HardConstraintParameter::DEFAULT_MAX_PICK_NODE_COUNT) {
    this->hc_manager->add_constr(new HcMaxPickNodeCount(parameter->max_pick_node_count));
  }
  // == hard constraints: max pick node count
  if (parameter->max_drop_node_count > HardConstraintParameter::DEFAULT_MAX_DROP_NODE_COUNT) {
    this->hc_manager->add_constr(new HcMaxDropNodeCount(parameter->max_drop_node_count));
  }

  // cost constraints
  // == cost constraints: dist
  if (parameter->cc_constr_dist_factor > CostConstraintParameter::CC_DIST_DEFAULT_DIST_FACTOR) {
    this->cc_manager->add_constr(new CcDist(parameter->cc_constr_dist_factor));
  }
}

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
  auto dim_vals = scenario->dimension_manager->empty_dim_values();
  auto labelset_value = scenario->label_manager->order_labelset->empty_labelset_value();
  auto labelset_value_bitset =
      scenario->label_manager->order_labelset->empty_labelset_value_bitset();
  auto cargo_orders = std::vector<const CargoOrder*>({
      cargo_order,
  });
  auto available_vehicle_bitset = scenario->carrier_manager->full_vehicle_bitset();

  auto order = new Order(cargo_orders, dim_vals, labelset_value, std::move(labelset_value_bitset),
                         std::move(available_vehicle_bitset));
  auto orders = std::vector<Order*>({order});

  auto infeasible_cargo_order = std::make_unique<InfeasibleCargoOrder>(cargo_orders);

  const auto& vehicles = scenario->carrier_manager->vehicles;
  bool has_feasible_flag = false;
  for (auto &vehicle : vehicles) {
    auto cur_load = this->construct_load_by_order(orders, vehicle);
    if (cur_load->constr_profile->infesible) {
      // TODO 添加不可解原因
    } else {
      has_feasible_flag = true;
      break;
    }
  }

  delete order;

  if (has_feasible_flag) {
    return nullptr;
  }

  return infeasible_cargo_order;
}
