/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <ctime>
#include <vector>

#include "bdm_dimension.h"
#include "bdm_time_window.h"
#include "c_constant.h"

/**
 * @brief 工作日历
 * @details 不同类型的工作时间段的基类
 */
class Calendar {
 public:
  /**
   * @brief 日历类型
   * @details 支持按天、按周的时间类型;
   */
  CalendarTimeRangeType time_range_type;
  /**
   * @brief 日期的时间窗列表
   * @details
   * 每个元素为一个日期的时间窗列表，每个时间窗为一个pair<int,
   * int>，分别表示开始时间和结束时间，单位: s 例如: {{0, 3600}, {3600, 7200}}
   * 表示该日期有2个时间窗，第一个时间窗从0开始，持续3600s，第二个时间窗从3600开始，持续7200s
   */
  std::vector<std::vector<std::pair<int, int>>> day_time_ranges;

  Calendar(CalendarTimeRangeType time_range_type);

  ~Calendar() = default;

  /**
   * @brief 设置某一天的时间窗列表
   */
  void set_ond_day_time_ranges(const int day_ind, std::vector<std::pair<int, int>>& time_ranges);
  /**
   * @brief 时间窗和工作日历取交集
   *
   * @param time_window 时间窗
   * @return 交集时间窗-timestamp格式
   */
  std::vector<TimeWindow*> intersection(TimeWindow* tw);
};

class WorkEffect {
 public:
  /**
   * @brief 工作效率表
   * @details 二维表<ActivityType, Dimension,
   * Quantity>，表示该维度在不同作业类型和维度每小时处理的货物数量
   */
  std::vector<std::vector<double>> effect_map;
  /**
   * @brief 维度数量
   */
  const int dims_len;

  WorkEffect(const DimensionManager* dim_manager);

  void add(const Dimension* dim, ActivityType activity_type, double quantity);

  long get_work_time(ActivityType activity_type, const std::vector<long>& dims_val) const;

  ~WorkEffect() = default;
};

/**
 * @brief 工作计划
 * @details 包含提卸货固定处理时间、日历、作业效率;
 */
class WorkPlan {
 public:
  /**
   * @brief 固定提货时间，单位: s
   */
  int fixed_pick_time;
  /**
   * @brief 固定卸货时间，单位: s
   */
  int fixed_drop_time;
  /**
   * @brief 提货日历
   */
  Calendar* pick_calendar;
  /**
   * @brief 卸货日历
   */
  Calendar* drop_calendar;
  /**
   * @brief 限行日历
   */
  Calendar* restrict_calendar;
  /**
   * @brief 工作效率
   */
  WorkEffect* work_effect;

  WorkPlan(const DimensionManager* dim_manager);

  ~WorkPlan();
  /**
   * @brief 设置固定提货时间
   * @param fixed_pick_time 固定提货时间，单位: s
   */
  void set_fixed_pick_time(int fixed_pick_time);
  /**
   * @brief 设置固定卸货时间
   * @param fixed_drop_time 固定卸货时间，单位: s
   */
  void set_fixed_drop_time(int fixed_drop_time);

  /**
   * @brief 设置日历
   * @param calendar 日历
   * @param calendar_type 日历类型
   */
  void set_calendar(Calendar* calendar, CalendarType calendar_type);
};
