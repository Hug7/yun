/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <string>

#include "pdm_context.h"
#include "pdm_workspace.h"

class Solver {
 public:
  /**
   * @brief solver上下文
   */
  SolverContext* context;
  /**
   * @brief 订单池
   */
  OrderPool* order_pool;
  /**
   * @brief 工作空间
   */
  Workspace* workspace;

  /**
   * @brief 构造solver
   * @param root_dir 文件根目录
   * @param log_dir 全局日志目录
   * @param log_level 日志等级
   */
  explicit Solver(std::string root_dir, const std::string& log_dir, const std::string& log_level);

  ~Solver();

  /**
   * @brief 求解
   */
  void solve();
};
