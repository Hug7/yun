/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "algo_alns_repair.h"
#include "pdm_context.h"
#include "pdm_solution.h"

/**
 * @brief alns repair算子贪心修复
 */
class AlnsRepairGreedy : public AlnsRepairOperator {
 public:
  explicit AlnsRepairGreedy(const SolverContext* _context);

  void call(Solution* sol) override;
};
