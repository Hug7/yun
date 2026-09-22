/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#include "s_solver.h"

#include <memory>

#include "pdm_order_pool.h"
#include "pdm_workspace.h"
#include "s_precheck.h"
#include "stra_strategy.h"

// ====== implement of Load Solver ======
Solver::Solver(std::string root_dir, const std::string& log_dir, const std::string& log_level) {
  // 初始化solver上下文，包含：日志、scenario、parameter、problem
  this->context = new SolverContext(std::move(root_dir), log_dir, log_level);
  this->order_pool = nullptr;
  this->workspace = nullptr;
}

Solver::~Solver() {
  // release context
  delete this->context;
  // release order pool
  delete this->order_pool;
  // release workspace
  delete this->workspace;
}

void Solver::solve() {
  // precheck, 发现不可解的订单则提前退出
  const auto solver_precheck = new SolverPrecheck(this->context);
  const bool can_continue_flag = solver_precheck->call();
  delete solver_precheck;
  if (!can_continue_flag) {
    return;
  }
  // 构造订单池
  this->order_pool = new OrderPool(this->context);
  // 构造工作台
  this->workspace = new Workspace(this->context, order_pool);
  // 注册策略
  const auto strategy_manager = std::make_unique<StrategyManager>(this->workspace);
  // 执行策略
  strategy_manager->exec_script();
  // 结果转换并导出
  this->context->visual_manager->localization_plan_result(workspace->loads_view(),
                                                   workspace->unassigned_orders_view());
}
