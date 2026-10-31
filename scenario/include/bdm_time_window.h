/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <cstdint>
#include <ctime>
#include <memory>
#include <vector>

#include "c_constant.h"

/**
 * @brief time window
 * @details contain the start time and end time of the time window
 */
class TimeWindow {
 public:
  using UPtr = std::unique_ptr<TimeWindow>;
  using VecUPtr = std::vector<std::unique_ptr<TimeWindow>>;
  /**
   * @brief start time of the time window
   */
  const std::int64_t early;
  /**
   * @brief end time of the time window
   */
  const std::int64_t late;

  TimeWindow()
      : early(TimeWindowParameter::DEFAULT_EARLY_TIME),
        late(TimeWindowParameter::DEFAULT_LATE_TIME) {};

  TimeWindow(const std::int64_t early, const std::int64_t late) : early(early), late(late) {}

  TimeWindow(TimeWindow* other) : early(other->early), late(other->late) {}
};
