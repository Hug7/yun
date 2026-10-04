/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <vector>

#include "pdm_order.h"

/**
 * @brief 带顺序的订单片段
 * @details segment 是指多个 order 按照指定访问顺序的载体
 */
class Segment {
 public:
  /**
   * @brief 订单列表
   */
  std::vector<const Order*> orders;
  /**
   * @brief 订单数量
   */
  int order_count;

  explicit Segment(std::vector<const Order*> _orders)
      : orders(std::move(_orders)), order_count(static_cast<int>(this->orders.size())) {}

  ~Segment() = default;
};
