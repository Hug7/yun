/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <sol/sol.hpp>

#include "algo_alns_repair.h"
#include "c_random_utils.h"
#include "pdm_context.h"
#include "pdm_solution.h"

class AlnsRepairGreedyLuaField {
 public:
  static constexpr const char* RANDOM_SEED = "random_seed";
  static constexpr const char* TOP_RATE = "top_rate";
};

/**
 * @brief alns repair算子贪心修复
 */
class AlnsRepairGreedy : public AlnsRepairOperator {
 public:
  /**
   * @brief 随机种子
   */
  int random_seed{37};
  /**
   * @brief 随机数
   */
  RandomUtils::UPtr random;
  /**
   * @brief 选择 segment-load 组合方案的 top 比例
   */
  double top_rate{0.2};

  explicit AlnsRepairGreedy(const SolverContext* _context, ThreadPool* _thread_pool,
                            const sol::table& cfg);

  void execute(RepairMatrix* repair_matrix) override;
};
