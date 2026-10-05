/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#include "stra_strategy.h"

#include <spdlog/spdlog.h>

#include <filesystem>

#include "algo_alns_model.h"
#include "algo_alns_operator.h"
#include "algo_alns_parameter.h"
#include "algo_alns_ruin.h"
#include "stra_construct.h"
#include "visual_manager.h"

// ====== implement of StrategyManager::Private ======
void StrategyManager::register_common_func(Workspace* workspace) {
  const SolverContext* context = workspace->context;
  // 注册-日志
  this->lua.set_function(
      "log",
      sol::overload(

          [logger = context->logger](const std::string& msg) { logger->info("[lua] {}", msg); },

          [logger = context->logger](const std::string& msg, const int log_level) {
            if (log_level == 1) {
              logger->debug("[lua] {}", msg);
            } else if (log_level == 2) {
              logger->warn("[lua] {}", msg);
            } else if (log_level == 3) {
              logger->error("[lua] {}", msg);
            } else {
              logger->info("[lua] {}", msg);
            }
          }));
  // 注册-结果可视化
  this->lua.set_function("snapshot", [workspace](const std::string& target_dir) {
    workspace->context->visual_manager->localization_plan_result(
        workspace->loads_view(), workspace->unassigned_orders_view(), target_dir);
  });
}

void StrategyManager::register_construct_heuristic_func(Workspace* workspace) {
  // 注册-knn
  // --knn参数
  constexpr auto constructors = sol::constructors<KnnParameter(), KnnParameter(int)>();
  this->lua.new_usertype<KnnParameter>("KnnParameter", constructors, sol::call_constructor,
                                       constructors, "neighbor_count",
                                       &KnnParameter::neighbor_count);
  // --knn方法
  this->lua.set_function("construct_knn", [workspace](const sol::object& knn_parameter) {
    if (!knn_parameter.valid()) {
      // 入参为KnnParameter，不传参或传nil时用结构体里的默认值
      ConstructHeuristic::k_nearest_neighbor(workspace, KnnParameter());
    } else {
      ConstructHeuristic::k_nearest_neighbor(workspace, knn_parameter.as<KnnParameter>());
    }
  });
}

void StrategyManager::register_alns_func(Workspace* workspace) {
  // 注册-alns
  /**
   * Lua 侧配置示例：
   * ruin_operator_params = {
   *   { code = "RuinRandomLoad", params = { min_rate = 0.1, max_rate = 0.3 } },
   *   { code = "RuinRandomLoad", params = { min_rate = 0.4, max_rate = 0.6 } },  -- 同算子不同参数
   *   { code = "RuinRandomLoad" }  -- params 可缺省，算子内部用默认值
   * }
   */
  this->lua.set_function("alns", [workspace](const sol::table& cfg) {
    // 解析alns参数
    const auto alns_parameter = parse_alns_parameter(cfg, workspace->context->logger);
    // 线程池寿命跟随本次 alns 调用, 不依赖 AlnsModel 的析构
    ThreadPool thread_pool{alns_parameter->tasks};
    // 解析alns算子
    const auto operator_manager =
        create_alns_operator(cfg, alns_parameter, workspace, &thread_pool);
    // 构建alns model
    const AlnsModel* alns_model =
        new AlnsModel(workspace, alns_parameter, operator_manager, &thread_pool);
    // 求解
    Solution* res_sol = alns_model->solve();
    // 将结果 res_sol 替换
    workspace->move_solution(res_sol);
    // 释放 alns_model
    delete alns_model;
  });
}

void StrategyManager::load_script() {
  // safe_script_file + pass_on_error：脚本出错时不抛异常，统一在下面判断
  const sol::protected_function_result load_result =
      this->lua.safe_script_file(this->strategy_file_path, sol::script_pass_on_error);
  if (!load_result.valid()) {
    const sol::error err = load_result;
    this->logger->error("策略脚本 {} 加载失败: {}", this->strategy_file_path, err.what());
    throw std::runtime_error(err.what());
  }
}

// ====== implement of StrategyManager::Public ======
StrategyManager::StrategyManager(Workspace* workspace) {
  const SolverContext* context = workspace->context;
  // 日志器随请求走，脚本相关的错误也要落进本次请求的日志文件
  this->logger = context->logger;
  // 拼装策略文件路径
  this->strategy_file_path = context->root_dir + "/" + context->parameter->strategy_file_name;
  // 校验策略文件是否存在
  if (!std::filesystem::is_regular_file(this->strategy_file_path)) {
    const std::string msg = "策略文件不存在，路径为: " + this->strategy_file_path;
    this->logger->error(msg);
    throw std::runtime_error(msg);
  }
  // 为lua虚拟机提供基本库
  this->lua.open_libraries(sol::lib::base, sol::lib::string, sol::lib::table, sol::lib::math);
  // 注册-公共方法
  this->register_common_func(workspace);
  // 注册-构造启发式方法
  this->register_construct_heuristic_func(workspace);
  // 注册-构造alns
  this->register_alns_func(workspace);
  // 加载脚本
  this->load_script();
}

void StrategyManager::exec_script() {
  const sol::protected_function solve = this->lua["solve"];
  const sol::protected_function_result call_result = solve();
  if (!call_result.valid()) {
    const sol::error err = call_result;
    this->logger->error("策略脚本 {} 运行失败: {}", this->strategy_file_path, err.what());
    throw std::runtime_error(err.what());
  }
}
