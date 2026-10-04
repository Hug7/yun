/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <vector>

#include "pdm_segment.h"
#include "pdm_solution.h"

/**
 * @brief segment 管理器
 * @details segment 是指多个order按照指定访问顺序的载体。
 */
class SegmentBuilder {
 public:
  /**
   * @brief 将未指派订单构造 segment 列表
   * TODO 1. 优先将绑定的订单构造成 segment
   * TODO 2. 学习 solution 中常出现的 segment
   * @param sol solution
   * @return segment 列表
   */
  [[nodiscard]] std::vector<Segment*> call(const Solution* sol);
};
