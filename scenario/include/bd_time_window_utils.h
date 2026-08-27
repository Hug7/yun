/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <vector>

#include "bd_time_window.h"
#include "bd_time_window_plan.h"

namespace TimeWindowInfer {
TimeWindowPlan::UPtr forward_s_s(TimeWindowPlan::UPtr& pre_node_plan_tw, const long cost_time,
                                  const long work_time, const TimeWindow* ori_tw);

void forward_s_m(TimeWindowPlan::UPtr& pre_node_plan_tw, const long cost_time, const long work_time,
                 std::vector<TimeWindow*>& ori_tws, TimeWindowPlan::VecUPtr& plan_node_tws);

TimeWindowPlan::VecUPtr forward(TimeWindowPlan::VecUPtr& pre_node_plan_tws, const long cost_time,
                                const long work_time, std::vector<TimeWindow*>& ori_tws);
}  // namespace TimeWindowInfer

namespace TimeWindowUntils {
/**
 * @brief get intersection of two time windows
 * @details if tw_a and tw_b are not intersected, return nullptr
 */
TimeWindow* intersection(const TimeWindow* tw_a, const TimeWindow* tw_b);
/**
 * @brief 对多段时间窗求交集
 * @details 如果没有交集, 返回空数组. 已知std::vector<TimeWindow*>是时间连续的
 */
std::vector<TimeWindow*> intersection_tws_arr(std::vector<std::vector<TimeWindow*>>& tws_arr);
/**
 * @brief merge continuous time windows
 */
std::vector<TimeWindow*> merge_time_windows(std::vector<TimeWindow*>& tws);
}  // namespace TimeWindowUntils
