/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#include "stra_strategy.h"

#include <spdlog/spdlog.h>

#include <filesystem>

#include "stra_construct.h"

// ====== implement of StrategyManager::Private ======
void StrategyManager::register_common_func(Workspace* workspace) {
  const SolverContext* context = workspace->context;
  // 注册-日志
  this->lua_vm.set_function("log_info", [logger = context->logger](const std::string& msg) {
    logger->info("[lua] {}", msg);
  });
  this->lua_vm.set_function("log_debug", [logger = context->logger](const std::string& msg) {
    logger->debug("[lua] {}", msg);
  });
  this->lua_vm.set_function("log_warn", [logger = context->logger](const std::string& msg) {
    logger->warn("[lua] {}", msg);
  });
  this->lua_vm.set_function("log_error", [logger = context->logger](const std::string& msg) {
    logger->error("[lua] {}", msg);
  });
}

void StrategyManager::register_construct_heuristic_func(Workspace* workspace) {
  // 注册-knn
  // --knn参数
  constexpr auto constructors = sol::constructors<KnnParameter(), KnnParameter(int)>();
  this->lua_vm.new_usertype<KnnParameter>("KnnParameter", constructors, sol::call_constructor,
                                          constructors, "neighbor_count",
                                          &KnnParameter::neighbor_count);
  // --knn方法
  this->lua_vm.set_function("construct_knn", [workspace](const sol::object& knn_parameter) {
    if (!knn_parameter.valid()) {
      // 入参为KnnParameter，不传参或传nil时用结构体里的默认值
      ConstructHeuristic::k_nearest_neighbor(workspace, KnnParameter());
    } else {
      ConstructHeuristic::k_nearest_neighbor(workspace, knn_parameter.as<KnnParameter>());
    }
  });
}

void StrategyManager::load_script() {
  // safe_script_file + pass_on_error：脚本出错时不抛异常，统一在下面判断
  const sol::protected_function_result load_result =
      this->lua_vm.safe_script_file(this->strategy_file_path, sol::script_pass_on_error);
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
  this->lua_vm.open_libraries(sol::lib::base, sol::lib::string, sol::lib::table, sol::lib::math);
  // 注册-公共方法
  this->register_common_func(workspace);
  // 注册-构造启发式方法
  this->register_construct_heuristic_func(workspace);
  // 加载脚本
  this->load_script();
}

void StrategyManager::exec_script(const SolverContext* context) {
  const sol::protected_function solve = this->lua_vm["solve"];
  const sol::protected_function_result call_result = solve();
  if (!call_result.valid()) {
    const sol::error err = call_result;
    this->logger->error("策略脚本 {} 运行失败: {}", this->strategy_file_path, err.what());
    throw std::runtime_error(err.what());
  }
}
