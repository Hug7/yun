/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <vector>

#include "pdm_context.h"
#include "pdm_order.h"

/**
 * @brief 构造problem层订单
 * @details 根据cargo order构造订单，使用合并手段减少问题规模。合并规则：
 * 1. cargo order的提货站点和卸货站点必须相同；
 * 2. 如果设置按照label合并，支持两种合并模式：
 * 2.1 按照label value种类分组，e.g. [A] [B] [C] [D]；
 * 2.2 指定label value组合合并, e.g. [A, B], [C, D]；
 * 3. 最重要的合并后要有可行解，要能被最小可用车型配送，也就是
 * 说合并变成最小化分组数量的多背包问题
 * @param context solver上下文
 * @return 订单集合，所有权交给调用方（如 Workspace），由调用方负责释放
 */
[[nodiscard]] std::vector<Order*> build_orders(const SolverContext* context);
