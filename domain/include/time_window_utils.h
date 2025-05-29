#ifndef TIME_WINDOW_UTILS_H
#define TIME_WINDOW_UTILS_H

#include <vector>
#include <algorithm>
#include <memory>

#include "time_window.h"
#include "time_window_plan.h"

namespace TimeWindowUtils
{
    inline std::unique_ptr<TimeWindowPlan> forward_infer(const std::unique_ptr<TimeWindowPlan> &pre_node_plan_tw, long cost_time, long work_time, TimeWindow *&ori_tw)
    {
        std::unique_ptr<TimeWindowPlan> cur_tw = std::make_unique<TimeWindowPlan>();

        long plan_early_arr = pre_node_plan_tw->early_dest + cost_time;
        long plan_late_arr = pre_node_plan_tw->late_dest + cost_time;

        if (plan_early_arr <= ori_tw->late)
        {
            if (plan_late_arr >= ori_tw->early)
            {
                cur_tw->early_arr = std::max(plan_early_arr, ori_tw->early);
                cur_tw->late_arr = std::min(plan_late_arr, ori_tw->late);
                cur_tw->early_dest = cur_tw->early_arr + work_time;
                cur_tw->late_dest = cur_tw->late_arr + work_time;
            }
            else
            {
                // wait time case
                cur_tw->early_arr = plan_late_arr;
                cur_tw->late_arr = plan_late_arr;
                cur_tw->early_dest = ori_tw->early + work_time;
                cur_tw->late_dest = ori_tw->early + work_time;
                cur_tw->wait_time = ori_tw->early - plan_late_arr;
            }
        }
        else
        {
            // over time case
            cur_tw->early_arr = plan_early_arr;
            cur_tw->late_arr = plan_early_arr;
            cur_tw->early_dest = plan_early_arr + work_time;
            cur_tw->late_dest = plan_early_arr + work_time;
            cur_tw->over_time = plan_early_arr - ori_tw->late;
        }

        return std::move(cur_tw);
    }

    inline void forward_infer(const std::unique_ptr<TimeWindowPlan> &pre_node_plan_tw, long cost_time, long work_time, std::vector<TimeWindow *> &ori_tws, std::vector<std::unique_ptr<TimeWindowPlan>> &plan_node_tws)
    {
        bool non_wait_over_flag = false;
        if (plan_node_tws.size() > 1 || (plan_node_tws.size() > 0 && plan_node_tws[0]->is_zero_wait_over_time()))
        {
            non_wait_over_flag = true;
        }

        long plan_early_arr = pre_node_plan_tw->early_dest + cost_time;
        long plan_late_arr = pre_node_plan_tw->late_dest + cost_time;

        for (TimeWindow *&cur_ori_tw : ori_tws)
        {
            if (plan_late_arr < cur_ori_tw->early)
            {
                // case: wait time
                if (non_wait_over_flag)
                {
                    break;
                }
                if (plan_node_tws.size() == 0 || (plan_node_tws[0]->over_time > 0 || (cur_ori_tw->early - plan_late_arr) < plan_node_tws[0]->wait_time))
                {
                    plan_node_tws.clear();
                    plan_node_tws.push_back(forward_infer(pre_node_plan_tw, cost_time, work_time, cur_ori_tw));
                }
            }else if (plan_early_arr > cur_ori_tw->late)
            {
                // case: over time
                if (non_wait_over_flag)
                {
                    continue;
                }
                if (plan_node_tws.size() == 0 || (plan_node_tws[0]->wait_time == 0 && (plan_early_arr - cur_ori_tw->late) < plan_node_tws[0]->over_time))
                {
                    plan_node_tws.clear();
                    plan_node_tws.push_back(forward_infer(pre_node_plan_tw, cost_time, work_time, cur_ori_tw));
                }
            } else
            {
                if (!non_wait_over_flag)
                {
                    plan_node_tws.clear();
                    non_wait_over_flag = true;
                }
                plan_node_tws.push_back(forward_infer(pre_node_plan_tw, cost_time, work_time, cur_ori_tw));
            }
            
        }
    }

    inline std::vector<std::unique_ptr<TimeWindowPlan>> forward_infer(std::vector<std::unique_ptr<TimeWindowPlan>> &pre_node_plan_tws, long cost_time, long work_time, std::vector<TimeWindow *> &ori_tws)
    {
        std::vector<std::unique_ptr<TimeWindowPlan>> plan_node_tws;
        for (const std::unique_ptr<TimeWindowPlan> &pre_node_plan_tw : pre_node_plan_tws) {
            forward_infer(pre_node_plan_tw, cost_time, work_time, ori_tws, plan_node_tws);
        }
        return std::move(plan_node_tws);
    }
}

#endif // TIME_WINDOW_UTILS_H
