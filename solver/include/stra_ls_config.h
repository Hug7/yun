/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <spdlog/spdlog.h>

#include <memory>
#include <sol/sol.hpp>

#include "algo_ls.h"

/**
 * @brief local search 策略的结构化配置
 * @details 字段由 algorithm 层的 `LsSearchConfig` 定义, solver 层只负责 Lua→POD 解析;
 *          这里保留策略层命名, 算法层只消费 `LsSearchConfig`
 */
using LsStrategyConfig = LsSearchConfig;

/**
 * @brief 解析 Lua 中 local search 的配置
 * @param cfg lua 传给 `local_search` 策略方法或 `alns.local_search` 子表的配置, 允许 nil
 * @param defaults 未显式配置字段的默认值
 * @param logger 日志器
 * @return 搜索参数与算子实例参数
 * @details 只负责 Lua 语法与形状: 键名、类型读取、算子 code 分发; 缺省字段由 `defaults`
 *          与算子参数结构体的默认值补齐。未知 code / 字段类型错误抛 `std::invalid_argument`。
 */
[[nodiscard]] LsStrategyConfig parse_local_search_strategy_config(
    const sol::object& cfg, const LsParameter& defaults,
    const std::shared_ptr<spdlog::logger>& logger);
