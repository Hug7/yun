/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

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
  long early_arr;
  /**
   * late arrive time
   */
  long late_arr;
  /**
   * early departure time
   */
  long early_depart;
  /**
   * late departure time
   */
  long late_depart;
  /**
   * wait time
   */
  long wait_time;
  /**
   * over time
   */
  long over_time;

  TimeWindowPlan()
      : early_arr(0), late_arr(0), early_depart(0), late_depart(0), wait_time(0), over_time(0) {};

  TimeWindowPlan(const long early_arr, const long late_arr, const long early_depart,
                 const long late_depart, long wait_time, const long over_time)
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
