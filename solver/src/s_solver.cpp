/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#include "s_solver.h"

#include <memory>

#include "s_precheck.h"
#include "pdm_order_pool.h"
#include "stra_strategy.h"

// ====== implement of Load Solver ======
Solver::Solver(std::string root_dir, const std::string& log_dir, const std::string& log_level) {
  // 初始化solver上下文，包含：日志、scenario、parameter、problem
  this->context = new SolverContext(std::move(root_dir), log_dir, log_level);
}

Solver::~Solver() {
  // release context
  delete this->context;
}

void Solver::solve() const {
  // precheck, 发现不可解的订单则提前退出
  const auto solver_precheck = new SolverPrecheck(this->context);
  const bool can_continue_flag = solver_precheck->call();
  delete solver_precheck;
  if (!can_continue_flag) {
    return;
  }
  // 构造订单池
  OrderPool* order_pool = new OrderPool(this->context);
  // 注册策略
  const auto strategy_manager = std::make_unique<StrategyManager>(this->context);
  // 执行策略
  strategy_manager->exec_script(this->context);
  // 结果转换并导出
}
