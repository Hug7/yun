/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <cstdint>
#include <vector>

#include "bdm_time_window.h"

class TimeWindowPlan {
 public:
  /**
   * early arrive time
   */
  std::int64_t early_arr{0};
  /**
   * late arrive time
   */
  std::int64_t late_arr{0};
  /**
   * early departure time
   */
  std::int64_t early_depart{0};
  /**
   * late departure time
   */
  std::int64_t late_depart{0};
  /**
   * wait time
   */
  std::int64_t wait_time{0};
  /**
   * over time
   */
  std::int64_t over_time{0};

  TimeWindowPlan() = default;

  TimeWindowPlan(std::int64_t early_arr, std::int64_t late_arr, std::int64_t early_depart,
                 std::int64_t late_depart, std::int64_t wait_time, std::int64_t over_time);

  TimeWindowPlan(const TimeWindow& time_window, std::int64_t work_time);

  explicit TimeWindowPlan(const TimeWindow& time_window);

  [[nodiscard]] bool is_zero_wait_over_time() const;

  [[nodiscard]] bool is_zero_wait_time() const;

  [[nodiscard]] bool is_zero_over_time() const;
};

namespace TimeWindowPlanFactory {
/**
 * @brief 获取默认的时间窗计划
 * @details 使用最宽的时间窗，即时间窗时间的取值上下限
 * @return 时间窗计划
 */
[[nodiscard]] TimeWindowPlan default_time_window_plan();

/**
 * @brief 获取默认的时间窗计划列表
 * @details 使用最宽的时间窗，即时间窗时间的取值上下限
 * @return 时间窗计划列表
 */
[[nodiscard]] std::vector<TimeWindowPlan> default_time_window_plans();

/**
 * @brief 根据时间窗构造时间窗计划列表
 */
[[nodiscard]] std::vector<TimeWindowPlan> create_time_window_plans(
    const std::vector<TimeWindow>& time_windows);
}  // namespace TimeWindowPlanFactory
