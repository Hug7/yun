/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <vector>

#include "pdm_context.h"
#include "pdm_order.h"

/**
 * @brief 订单池-problem层
 * @details 根据cargo order构造订单池，使用合并手段减少问题规模
 */
class OrderPool {
 public:
  /**
   * @brief 订单集合
   */
  std::vector<Order*> orders;
  /**
   * @brief 订单数量
   */
  int len;

  explicit OrderPool(const SolverContext* context);

  ~OrderPool();
};
