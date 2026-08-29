/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#include "s_solver.h"

#include <memory>
#include <vector>

#include <spdlog/spdlog.h>

#include "cc_dist.h"
#include "pdm_infeasible_order.h"
#include "hc_label.h"
#include "hc_location.h"
#include "hc_vehicle.h"
#include "pr_problem.h"
#include "sc_dist.h"
#include "scr_standard_csv_reader.h"

// ====== implement of Load Solver ======
Solver::~Solver() {
  // release scenario
  delete this->scenario;
}

void Solver::load_scenario() {
  auto reader = std::make_unique<StandardCsvReader>(this->roo_dir);
  this->scenario = reader->loading_scenario();
}

void Solver::load_parameter() {
  this->parameter = new Parameter();
  // TODO 实现读取参数

  // 参数后处理
  this->parameter->post_process(this->scenario);
}

void Solver::create_problem() {
  // TODO 先默认创建 VRP
  this->problem = new Problem(this->scenario, this->parameter);

  // hard constraints: base
  this->problem->hc_manager->add_constr(new HcVehicleCapacity());
  this->problem->hc_manager->add_constr(new HcAvailableVehicle());

  // add hard constraints by scenario
  auto parameter = this->problem->parameter;
  // -- hard constraints: max pick node count
  if (parameter->time_window_constr_enabled) {
    this->problem->hc_manager->add_constr(new HcTimeWindow());
  }
  // -- hard constraints: max drop node count
  if (parameter->max_pick_node_count > HardConstraintParameter::DEFAULT_MAX_PICK_NODE_COUNT) {
    this->problem->hc_manager->add_constr(new HcMaxPickNodeCount(parameter->max_pick_node_count));
  }
  // -- hard constraints: max pick node count
  if (parameter->max_drop_node_count > HardConstraintParameter::DEFAULT_MAX_DROP_NODE_COUNT) {
    this->problem->hc_manager->add_constr(new HcMaxDropNodeCount(parameter->max_drop_node_count));
  }
  // soft constraints
  if (parameter->sc_constr_dist_factor > SoftConstraintParameter::SC_DIST_DEFAULT_DIST_FACTOR) {
    this->problem->sc_manager->add_constr(new ScDist(parameter->sc_constr_dist_factor));
  }
  // cost constraints
  // -- cost constraints: dist
  if (parameter->cc_constr_dist_factor > CostConstraintParameter::CC_DIST_DEFAULT_DIST_FACTOR) {
    this->problem->cc_manager->add_constr(new CcDist(parameter->cc_constr_dist_factor));
  }
}

bool Solver::precheck() {
  this->infeasible_cargo_orders = std::vector<InfeasibleCargoOrder::UPtr>();
  for (const auto& cargo_order : this->scenario->cargo_order_manager->cargo_orders) {
    auto infeasible_cargo_order = this->problem->check_feasibility(cargo_order);
    if (infeasible_cargo_order == nullptr) {
      continue;
    }
    this->infeasible_cargo_orders.push_back(std::move(infeasible_cargo_order));
  }

  if (!this->infeasible_cargo_orders.empty()) {
      spdlog::warn("number of cargo orders {} are infeasible!", this->infeasible_cargo_orders.size());
  }
  
  return this->infeasible_cargo_orders.empty();
}
