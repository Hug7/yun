/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "algo_alns_repair.h"
#include "c_random_utils.h"
#include "pdm_context.h"

/**
 * @brief alns repair算子`RepairGreedy`的参数
 */
struct AlnsRepairGreedyParam {
  /**
   * @brief 随机种子
   */
  int random_seed{37};
  /**
   * @brief 选择 segment-load 组合方案的 top 比例
   */
  double top_rate{0.2};
};

/**
 * @brief alns repair算子贪心修复
 */
class AlnsRepairGreedy : public AlnsRepairOperator {
 public:
  /**
   * @brief 结构化参数
   */
  const AlnsRepairGreedyParam parameter;
  /**
   * @brief 随机数
   */
  RandomUtils::UPtr random;

  explicit AlnsRepairGreedy(const SolverContext* _context, ParallelExecutor* _parallel_executor,
                            const AlnsRepairGreedyParam& _parameter);

  void execute(RepairMatrix* repair_matrix) override;

  [[nodiscard]] std::string format_parameters() const override;
};
