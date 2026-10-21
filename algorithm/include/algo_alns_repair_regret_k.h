/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <sol/sol.hpp>

#include "algo_alns_repair.h"
#include "pdm_context.h"

/**
 * @brief alns repair算子`RepairRegretK`的参数在lua中的字段集合
 */
class AlnsRepairRegretKLuaField {
 public:
  static constexpr const char* REGRET_K = "regret_k";
};

/**
 * @brief alns repair算子后悔式修复(Regret-K)
 * @details 每轮按稀缺性层级和后悔值选择一个未指派 segment, 落实其最优插入方案;
 * 当所有未指派 segment 都没有可行插入方案时, 沿用贪心修复的新建车次兜底。
 */
class AlnsRepairRegretK : public AlnsRepairOperator {
 public:
  /**
   * @brief 后悔值阶数
   */
  int regret_k{2};

  explicit AlnsRepairRegretK(const SolverContext* _context, ParallelExecutor* _parallel_executor,
                             const sol::table& cfg);

  void execute(RepairMatrix* repair_matrix) override;
};
