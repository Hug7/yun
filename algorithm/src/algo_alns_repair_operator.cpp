/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#include "algo_alns_repair_operator.h"

#include <memory>
#include <utility>
#include <vector>

#include "algo_segment_builder.h"
#include "pdm_solution.h"

// ====== implement of AlnsRepairOperator ======
void AlnsRepairOperator::call(Solution* sol, SegmentBuilder* segment_builder) {
  // 生成 segments
  auto segments = segment_builder->call(sol);
  // 构造 load 和 segment 的成本矩阵
  const auto repair_matrix = std::make_unique<RepairMatrix>(this->parallel_executor, this->context,
                                                            sol, std::move(segments));
  repair_matrix->init();
  // 执行 repair 操作
  this->execute(repair_matrix.get());
}
