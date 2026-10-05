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

  this->load_context = new LoadContext(this->scenario, this->parameter);

  this->hc_manager = new HardConstraintManager();
  this->sc_manager = new SoftConstraintManager();
  this->cc_manager = new CostConstraintManager();
}

Problem::~Problem() {
  delete this->lu_policy;
  delete this->pd_pattern;
  delete this->load_context;
  delete this->hc_manager;
  delete this->sc_manager;
  delete this->cc_manager;
}

void Problem::eval_load(Load* load) const {
  // todo 是否需要重置待讨论
  const LoadConstrProfile::UPtr& constr_profile = load->constr_profile;
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

void Problem::tmp_select_best_vehicle(Load* load, VehicleResource::UPtr& vehicle_resource) const {
  const Vehicle* best_vehicle = load->vehicle;
  double best_obj_val = std::numeric_limits<double>::max();
  if (load->is_feasible()) {
    best_obj_val = load->constr_profile->obj_val;
  }
  for (const auto& vehicle_usage : vehicle_resource->vehicle_usages) {
    if (vehicle_usage->vehicle == best_vehicle) {
      continue;
    }
    if (vehicle_usage->has_available_vehicle()) {
      load->change_vehicle(vehicle_usage->vehicle);
      this->eval_load(load);
      if (load->is_feasible() && load->constr_profile->obj_val < best_obj_val) {
        best_vehicle = vehicle_usage->vehicle;
        best_obj_val = load->constr_profile->obj_val;
      }
    }
  }
  // 切换最优车型
  load->change_vehicle(best_vehicle);
  this->eval_load(load);
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

Load* Problem::construct_load_by_order(std::vector<const Order*>& orders,
                                       VehicleResource::UPtr& vehicle_resource) const {
  Load* load = this->pd_pattern->create_load(this->load_context);
  for (const auto order : orders) {
    this->pd_pattern->insert_last_delivery(load, order);
  }
  // try using different vehicle
  LoadConstrProfile::UPtr best_profile = nullptr;
  const Vehicle* best_vehicle = nullptr;
  for (const auto& vehicle_usage : vehicle_resource->vehicle_usages) {
    // check vehicle resource
    if (!vehicle_usage->has_available_vehicle()) {
      continue;
    }
    // change vehicle
    load->change_vehicle(vehicle_usage->vehicle);
    // temporary evaluate of load
    auto cur_constr_profile = this->tmp_eval_load(load);
    if (cur_constr_profile->is_infeasible()) {
      continue;
    }
    if (best_vehicle == nullptr || cur_constr_profile->dominate(best_profile)) {
      best_profile = std::move(cur_constr_profile);
      best_vehicle = vehicle_usage->vehicle;
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
    this->pd_pattern->insert_last_delivery(load, order);
  }
  load->change_vehicle(vehicle);
  this->eval_load(load);

  return load;
}

Load* Problem::greedy_insert_segment_to_load(Load* ori_load, Segment* segment,
                                             VehicleResource::UPtr& vehicle_resource) const {
  // 只拷贝一次: 后续所有试插都在这份副本上进行, 每次试插后回滚
  Load* load = this->pd_pattern->deep_copy_load(ori_load);
  const Vehicle* ori_vehicle = load->vehicle;
  std::vector<InsertSlot> slots = this->pd_pattern->enumerate_insert_slots(load, segment);

  const Vehicle* best_vehicle = nullptr;
  double best_obj_val = std::numeric_limits<double>::max();
  int best_slot_ind = -1;
  const int slot_len = static_cast<int>(slots.size());

  for (int u = 0; u < slot_len; u++) {
    // 车型每轮重置: 否则上一轮选出的车型会泄漏到本轮, 结果依赖枚举顺序
    load->reset_route_profile();
    const std::vector<Activity*> inserted =
        this->pd_pattern->insert_segment_at(load, segment, slots[u]);
    if (inserted.empty()) {
      continue;
    }
    this->eval_load(load);
    // 如果是车辆导致的不可行，只挑选最优车型, 不占用车辆资源
    if (load->is_infeasible()) {
      this->tmp_select_best_vehicle(load, vehicle_resource);
    }
    // 同分时优先保留原车型
    if (load->is_feasible()) {
      const double obj_val = load->constr_profile->obj_val;
      if (load->constr_profile->obj_val < best_obj_val) {
        best_vehicle = load->vehicle;
        best_obj_val = obj_val;
        best_slot_ind = u;
      }
    }
    // 回滚链表结构; travel_dist/travel_time/ptws 不必恢复, 每次 eval 都会全量重算
    load->remove_activities(inserted);
    load->change_vehicle(ori_vehicle);
  }

  if (best_vehicle == nullptr) {
    delete load;
    return nullptr;
  }
  // 最优结构已在回滚中销毁: 用记录的槽位和车型重做一次插入
  load->reset_route_profile();
  load->change_vehicle(best_vehicle);
  if (this->pd_pattern->insert_segment_at(load, segment, slots[best_slot_ind]).empty()) {
    throw std::runtime_error("invalid insertion scheme in function `greedy_insert_segment_to_load`");
  }
  this->eval_load(load);

  if (load->is_infeasible()) {
    throw std::runtime_error("插入segment后出现无解 in function `greedy_insert_segment_to_load`");
  }

  return load;
}
