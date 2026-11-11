/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <spdlog/spdlog.h>

#include <memory>
#include <sol/sol.hpp>
#include <vector>

#include "algo_alns_operator.h"
#include "algo_alns_parameter.h"

/**
 * @brief ALNS 策略的结构化配置
 * @details 由策略层一次性完成 Lua→POD 转换, 算法层只消费这里的结构化参数
 */
struct AlnsStrategyConfig {
  /**
   * @brief ALNS 全局参数
   */
  std::unique_ptr<AlnsParameter> parameter;
  /**
   * @brief ruin 算子参数, 为空表示使用算法层的默认 ruin 算子
   */
  std::vector<AlnsRuinOperatorParam> ruin_operator_params;
  /**
   * @brief repair 算子参数, 为空表示使用算法层的默认 repair 算子
   */
  std::vector<AlnsRepairOperatorParam> repair_operator_params;
};

/**
 * @brief 解析 Lua 中的 alns 配置
 * @param cfg lua 中传给 alns 策略方法的配置表
 * @param logger 日志器
 * @return 全局参数与每类算子的结构化参数
 * @details 只负责 Lua 语法与形状: 键名、类型读取、算子 code 分发、缺省字段
 *          由算子参数结构体的默认值补齐; 算子参数之间的语义校验仍由算法层完成
 */
AlnsStrategyConfig parse_alns_strategy_config(const sol::table& cfg,
                                              const std::shared_ptr<spdlog::logger>& logger);
