/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <memory>
#include <vector>

#include "bd_time_window.h"

class TimeWindowPlan {
 public:
  using UPtr = std::unique_ptr<TimeWindowPlan>;
  using VecUPtr = std::vector<UPtr>;
  /**
   * early arrive time
   */
  long early_arr;
  /**
   * late arrive time
   */
  long late_arr;
  /**
   * early destination time
   */
  long early_dest;
  /**
   * late destination time
   */
  long late_dest;
  /**
   * wait time
   */
  long wait_time;
  /**
   * over time
   */
  long over_time;

  TimeWindowPlan()
      : early_arr(0), late_arr(0), early_dest(0), late_dest(0), wait_time(0), over_time(0) {};

  TimeWindowPlan(long early_arr, long late_arr, long early_dest, long late_dest, long wait_time,
                 long over_time)
      : early_arr(early_arr),
        late_arr(late_arr),
        early_dest(early_dest),
        late_dest(late_dest),
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
TimeWindowPlan::VecUPtr create_time_window_plans(const std::vector<TimeWindow*> &time_windows);
}  // namespace TimeWindowPlanFactory
