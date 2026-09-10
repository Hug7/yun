/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "pdm_context.h"
#include "pdm_infeasible_order.h"

/**
 * @brief 预校验
 */
class SolverPrecheck {
 private:
  /**
   * @brief solver的上下文
   */
  SolverContext* solver_context;
  /**
   * @brief 不可解订单集合
   */
  InfeasibleCargoOrder::VecUPtr infeasible_cargo_orders;

 public:
  explicit SolverPrecheck(SolverContext* solver_context) : solver_context(solver_context) {}

  /**
   * @brief 预校验
   * @details 校验是否存在不满足约束的订单，如果存在则提前退出
   */
  bool call();
};
