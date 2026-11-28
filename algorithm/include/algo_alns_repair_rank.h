/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <vector>

#include "algo_alns_repair.h"

/**
 * @brief repair matrix 候选 选择的纯函数
 */
namespace RepairRank {

/**
 * @brief 从候选集合中选出全序最优的 k 个候选(delta 最小者最优)
 * @param candidates 候选集合, 允许重复(全序相同时视为等价)
 * @param k 保留数量, <=0 时返回空
 * @return 按全序升序排列的结果(front() 最优), 元素数量 = min(k, candidates.size())
 */
[[nodiscard]] std::vector<RepairCellView> select_top_k(std::vector<RepairCellView> candidates,
                                                       int k);

}  // namespace RepairRank
