/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <cstdint>
#include <memory>
#include <vector>

#include "bdm_time_window.h"

class TimeWindowPlan {
 public:
  using UPtr = std::unique_ptr<TimeWindowPlan>;
  using VecUPtr = std::vector<UPtr>;
  /**
   * early arrive time
   */
  std::int64_t early_arr;
  /**
   * late arrive time
   */
  std::int64_t late_arr;
  /**
   * early departure time
   */
  std::int64_t early_depart;
  /**
   * late departure time
   */
  std::int64_t late_depart;
  /**
   * wait time
   */
  std::int64_t wait_time;
  /**
   * over time
   */
  std::int64_t over_time;

  TimeWindowPlan()
      : early_arr(0), late_arr(0), early_depart(0), late_depart(0), wait_time(0), over_time(0) {};

  TimeWindowPlan(const std::int64_t early_arr, const std::int64_t late_arr,
                 const std::int64_t early_depart, const std::int64_t late_depart,
                 std::int64_t wait_time, const std::int64_t over_time)
      : early_arr(early_arr),
        late_arr(late_arr),
        early_depart(early_depart),
        late_depart(late_depart),
        wait_time(wait_time),
        over_time(over_time) {};

  TimeWindowPlan(const TimeWindow* time_window, int work_time);

  TimeWindowPlan(const TimeWindow* time_window);

  TimeWindowPlan::UPtr deep_copy();

  bool is_zero_wait_over_time();

  bool is_zero_wait_time();

  bool is_zero_over_time();
};

namespace TimeWindowPlanFactory {
/**
 * @brief 获取默认的时间窗计划
 * @details 使用最宽的时间窗，即时间窗时间的取值上下限
 * @return 时间窗计划
 */
TimeWindowPlan::UPtr default_time_window_plan();

/**
 * @brief 获取默认的时间窗计划列表
 * @details 使用最宽的时间窗，即时间窗时间的取值上下限
 * @return 时间窗计划列表
 */
TimeWindowPlan::VecUPtr default_time_window_plans();

/**
 * @brief 根据时间窗构造时间窗计划列表
 */
TimeWindowPlan::VecUPtr create_time_window_plans(const std::vector<TimeWindow*>& time_windows);
}  // namespace TimeWindowPlanFactory
