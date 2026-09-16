/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <sol/sol.hpp>

#include "pdm_workspace.h"

/**
 * @brief 策略管理器
 */
class StrategyManager {
 private:
  /**
   * @brief 策略文件路径
   */
  std::string strategy_file_path;
  /**
   * @brief lua虚拟机
   */
  sol::state lua_vm;
  /**
   * @brief 日志器，本次请求专属
   */
  std::shared_ptr<spdlog::logger> logger;

  /**
   * @brief 注册-公共方法
   * @param workspace 工作空间
   */
  void register_common_func(Workspace* workspace);
  /**
   * @brief 注册-构造启发式方法
   * @param workspace 工作空间
   */
  void register_construct_heuristic_func(Workspace* workspace);
  /**
   * @brief 加载脚本
   */
  void load_script();

 public:
  explicit StrategyManager(Workspace* workspace);

  /**
   * @brief 执行脚本
   * @param context solver上下文
   */
  void exec_script(const SolverContext* context);
};
