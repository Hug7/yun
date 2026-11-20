/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <cstdint>
#include <optional>
#include <vector>

#include "bdm_time_window.h"
#include "bdm_time_window_plan.h"

namespace TimeWindowInfer {
TimeWindowPlan forward_s_s(const TimeWindowPlan& pre_node_plan_tw, std::int64_t cost_time,
                           std::int64_t work_time, const TimeWindow& ori_tw);

void forward_s_m(const TimeWindowPlan& pre_node_plan_tw, std::int64_t cost_time,
                 std::int64_t work_time, const std::vector<TimeWindow>& ori_tws,
                 std::vector<TimeWindowPlan>& plan_node_tws);

std::vector<TimeWindowPlan> forward(const std::vector<TimeWindowPlan>& pre_node_plan_tws,
                                    std::int64_t cost_time, std::int64_t work_time,
                                    const std::vector<TimeWindow>& ori_tws);
}  // namespace TimeWindowInfer

namespace TimeWindowUntils {
/**
 * @brief get intersection of two time windows
 * @details if tw_a and tw_b are not intersected, return std::nullopt
 */
[[nodiscard]] std::optional<TimeWindow> intersection(const TimeWindow& tw_a,
                                                     const TimeWindow& tw_b);

/**
 * @brief 对多段时间窗求交集
 * @details 如果没有交集, 返回空数组. 已知 std::vector<TimeWindow> 是时间连续的
 */
[[nodiscard]] std::vector<TimeWindow> intersection_tws_arr(
    const std::vector<const std::vector<TimeWindow>*>& tws_arr);

/**
 * @brief merge continuous time windows
 * @details 入参按值接收(调用方用 std::move 传入), 结果按时间有序
 */
[[nodiscard]] std::vector<TimeWindow> merge_time_windows(const std::vector<TimeWindow>& tws);

/**
 * @brief 把 [early, late] 并入有序结果集, 与前一段间隔不超过阈值时合并
 */
void append_merged(std::vector<TimeWindow>& res, std::int64_t early, std::int64_t late);
}  // namespace TimeWindowUntils
