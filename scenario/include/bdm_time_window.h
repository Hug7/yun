/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <cstdint>

#include "c_constant.h"

/**
 * @brief time window
 * @details contain the start time and end time of the time window.
 * 纯值类型(16B), 仅由工厂与工具函数产出, 消费方一律以 const 引用接收
 */
class TimeWindow {
 public:
  /**
   * @brief start time of the time window
   */
  std::int64_t early{TimeWindowParameter::DEFAULT_EARLY_TIME};
  /**
   * @brief end time of the time window
   */
  std::int64_t late{TimeWindowParameter::DEFAULT_LATE_TIME};

  TimeWindow() = default;

  TimeWindow(const std::int64_t early, const std::int64_t late) : early(early), late(late) {}
};
