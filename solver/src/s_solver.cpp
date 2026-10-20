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
  // 允许重复调用: 先释放上一轮残留的订单池和工作台, 否则会泄漏
  delete this->workspace;
  this->workspace = nullptr;
  delete this->order_pool;
  this->order_pool = nullptr;
  // precheck, 发现不可解的订单则提前退出
  const auto solver_precheck = std::make_unique<SolverPrecheck>(this->context);
  if (!solver_precheck->call()) {
    this->context->logger->warn("precheck 因为存在订单未分配所以提前退出！");
    return;
  }
  // 构造订单池
  this->order_pool = new OrderPool(this->context);
  // 构造工作台
  this->workspace = new Workspace(this->context, order_pool);
  // 注册策略
  const auto strategy_manager = std::make_unique<StrategyManager>(this->workspace);
  // 开始solve函数计时
  auto solve_func_timing = std::make_unique<FuncTiming>("solve");
  // 执行策略
  strategy_manager->exec_script();
  // 记录solve函数计时
  this->workspace->add_func_timing(std::move(solve_func_timing));
  // 结果转换并导出
  this->context->visual_manager->localization_plan_result(this->workspace->loads_view(),
                                                          this->workspace->unassigned_orders_view(),
                                                          this->workspace->func_timings);
}
