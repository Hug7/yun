/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#include "stra_strategy.h"

#include <spdlog/spdlog.h>

#include <filesystem>

// ====== implement of StrategyManager::Private ======
void StrategyManager::register_clazz(const SolverContext* context) {
  // this->lua_vm.new_usertype
}

void StrategyManager::register_func(const SolverContext* context) {
  // this->lua_vm.set_function
  // 脚本日志走本次请求的 logger，才能同时进控制台和两个日志文件
  this->lua_vm.set_function(
      "log", [logger = context->logger](const std::string& msg) { logger->info("[lua] {}", msg); });
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
StrategyManager::StrategyManager(const SolverContext* context) {
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
  // 注册-类
  this->register_clazz(context);
  // 注册-方法
  this->register_func(context);
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
