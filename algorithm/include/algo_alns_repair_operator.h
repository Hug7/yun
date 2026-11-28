/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <magic_enum/magic_enum.hpp>
#include <string>

#include "algo_alns_repair.h"

class SegmentBuilder;
class Solution;
class SolverContext;

/**
 * @brief alns repair算子编码
 */
enum class AlnsRepairOperatorCode {
  RepairGreedy,
  RepairRegretK,
};

/**
 * @brief alns repair算子父类
 */
class AlnsRepairOperator {
 public:
  /**
   * @brief repair算子编码
   */
  const std::string code;
  /**
   * @brief solver上下文
   */
  const SolverContext* context;
  /**
   * @brief 并行执行器
   * @details 由外层 alns 调用方持有, 本类不负责释放
   */
  ParallelExecutor* parallel_executor;

  virtual ~AlnsRepairOperator() = default;

  explicit AlnsRepairOperator(const AlnsRepairOperatorCode operator_code,
                              const SolverContext* _context, ParallelExecutor* _parallel_executor)
      : code(std::string(magic_enum::enum_name(operator_code))),
        context(_context),
        parallel_executor(_parallel_executor) {}

  virtual void execute(RepairMatrix* repair_matrix) = 0;

  /**
   * @brief 格式化算子参数, 用于求解前打印
   * @return 形如 `a=1, b=2` 的参数文本; 无参数时返回空串
   */
  [[nodiscard]] virtual std::string format_parameters() const = 0;

  /**
   * @brief 构造 repair matrix 并执行本算子
   */
  void call(Solution* sol, SegmentBuilder* segment_builder);
};
