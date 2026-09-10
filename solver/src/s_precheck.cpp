/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#include "s_precheck.h"

#include <spdlog/spdlog.h>

// ====== implement of SolverPrecheck ======
bool SolverPrecheck::call() {
  this->infeasible_cargo_orders = std::vector<InfeasibleCargoOrder::UPtr>();
  for (const auto& cargo_order :
       this->solver_context->scenario->cargo_order_manager->cargo_orders) {
    auto infeasible_cargo_order = this->solver_context->problem->check_feasibility(cargo_order);
    if (infeasible_cargo_order == nullptr) {
      continue;
    }
    this->infeasible_cargo_orders.push_back(std::move(infeasible_cargo_order));
  }

  if (!this->infeasible_cargo_orders.empty()) {
    spdlog::warn("number of cargo orders {} are infeasible!", this->infeasible_cargo_orders.size());
    // 将不可解的订单和原因记录到csv文件中
    this->solver_context->visual_manager->infeasible_cargo_order_to_csv(this->infeasible_cargo_orders);
  }

  return this->infeasible_cargo_orders.empty();
}
