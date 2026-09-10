/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <string>

#include "pdm_context.h"

class Solver {
 public:
  /**
   * @brief solver上下文
   */
  SolverContext* solver_context;

  explicit Solver(std::string root_dir);

  ~Solver();

  /**
   * @brief 求解
   */
  void solve() const;
};
