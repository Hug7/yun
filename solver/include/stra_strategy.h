/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <sol/sol.hpp>

#include "pdm_context.h"

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
   * @brief 注册usertype：把类的方法暴露给脚本
   * @param context solver上下文
   */
  void register_clazz(const SolverContext* context);
  /**
   * @brief 注册方法
   * @param context solver上下文
   */
  void register_func(const SolverContext* context);
  /**
   * @brief 加载脚本
   */
  void load_script();

 public:
  explicit StrategyManager(const SolverContext* context);

  /**
   * @brief 执行脚本
   * @param context solver上下文
   */
  void exec_script(const SolverContext* context);
};
