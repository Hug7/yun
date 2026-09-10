/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#include "s_solver.h"

#include <spdlog/spdlog.h>

#include "s_precheck.h"
#include "pdm_order_pool.h"

// ====== implement of Load Solver ======
Solver::Solver(std::string root_dir) {
  this->solver_context = new SolverContext(std::move(root_dir));
}

Solver::~Solver() {
  // release context
  delete this->solver_context;
}

void Solver::solve() const {
  // precheck, 发现不可解的订单则
  const auto solver_precheck = new SolverPrecheck(this->solver_context);
  const bool can_continue_flag = solver_precheck->call();
  delete solver_precheck;
  if (!can_continue_flag) {
    return;
  }
  // 构造订单池
  OrderPool* order_pool = new OrderPool(this->solver_context);
  // 求解

  // 结果转换并导出

  // create a new plan
  // Plan* plan = new Plan(this->scenario);
}
