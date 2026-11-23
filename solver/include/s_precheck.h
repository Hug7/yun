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
  SolverContext* context;
  /**
   * @brief 不可解订单集合
   */
  InfeasibleCargoOrder::VecUPtr infeasible_cargo_orders;
  /**
   * @brief 校验单个货单能否被任一车辆承运
   * @details 用临时 Order 构造车次并求值；可行返回 nullptr，否则返回带无解原因的记录
   */
  [[nodiscard]] InfeasibleCargoOrder::UPtr check_cargo_order(CargoOrder* cargo_order) const;

 public:
  explicit SolverPrecheck(SolverContext* context) : context(context) {}

  /**
   * @brief 预校验
   * @details 校验是否存在不满足约束的订单，如果存在则提前退出
   */
  bool call();
};
