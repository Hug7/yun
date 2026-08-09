/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <vector>
#include <algorithm>
#include <memory>

#include "c_constant.h"
#include "bd_time_window.h"
#include "bd_time_window_plan.h"

namespace TimeWindowInfer
{
    TimeWindowPlan *forward_infer_s_s(const TimeWindowPlan *pre_node_plan_tw,
                                      long cost_time,
                                      long work_time,
                                      TimeWindow *ori_tw);

    void forward_infer_s_m(const TimeWindowPlan *pre_node_plan_tw,
                           long cost_time,
                           long work_time,
                           std::vector<TimeWindow *> &ori_tws,
                           std::vector<TimeWindowPlan *> &plan_node_tws);

    std::vector<TimeWindowPlan *> forward_infer(std::vector<TimeWindowPlan *> &pre_node_plan_tws,
                                                long cost_time,
                                                long work_time,
                                                std::vector<TimeWindow *> &ori_tws);
}

namespace TimeWindowUntils
{
    /**
     * @brief deep copy time window
     */
    TimeWindow *copy_time_window(TimeWindow *tw);
    /**
     * @brief get intersection of two time windows
     * @details if tw_a and tw_b are not intersected, return nullptr
     */
    TimeWindow *intersection(TimeWindow *tw_a, TimeWindow *tw_b);
    /**
     * @brief merge continuous time windows
     */
    std::vector<TimeWindow *> merge_time_windows(std::vector<TimeWindow *> tws);
}
