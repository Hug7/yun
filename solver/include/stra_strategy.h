/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <memory>
#include <sol/sol.hpp>

#include "algo_parallel_executor.h"
#include "pdm_workspace.h"

namespace StrategyKey {
constexpr char LOG[] = "log";
constexpr char SNAPSHOT[] = "snapshot";
constexpr char CONSTRUCT_KNN[] = "construct_knn";
constexpr char ALNS[] = "alns";
}  // namespace StrategyKey

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
  sol::state lua;
  /**
   * @brief 日志器，本次请求专属
   */
  std::shared_ptr<spdlog::logger> logger;
  /**
   * @brief 共享并行执行器
   */
  std::unique_ptr<ParallelExecutor> parallel_executor;
  /**
   * @brief 并行数
   * @details 为 -1 时 worker_count 为设备的最大线程数。
   * 可修改修改后会重建新的执行器，减少重复创建和释放
   */
  int parallel_worker_count{-1};

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
   * @brief 注册-alns的算法方法
   * @param workspace 工作空间
   */
  void register_alns_func(Workspace* workspace);
  /**
   * @brief 加载脚本
   */
  void load_script();
  /**
   * @brief 获取并行执行器
   * @param _parallel_worker_count 执行器并行数
   * @return 并行执行器
   */
  ParallelExecutor* get_parallel_executor(int _parallel_worker_count);

 public:
  explicit StrategyManager(Workspace* workspace);

  /**
   * @brief 执行脚本
   */
  void exec_script();
};
