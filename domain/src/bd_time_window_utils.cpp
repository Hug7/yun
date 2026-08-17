/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#include "bd_time_window_utils.h"

// ====== implement of TimeWindowInfer ======
TimeWindowPlan* TimeWindowInfer::forward_infer_s_s(const TimeWindowPlan* pre_node_plan_tw,
                                                   long cost_time, long work_time,
                                                   TimeWindow* ori_tw) {
  TimeWindowPlan* cur_tw = new TimeWindowPlan();

  long plan_early_arr = pre_node_plan_tw->early_dest + cost_time;
  long plan_late_arr = pre_node_plan_tw->late_dest + cost_time;

  if (plan_early_arr <= ori_tw->late) {
    if (plan_late_arr >= ori_tw->early) {
      cur_tw->early_arr = std::max(plan_early_arr, ori_tw->early);
      cur_tw->late_arr = std::min(plan_late_arr, ori_tw->late);
      cur_tw->early_dest = cur_tw->early_arr + work_time;
      cur_tw->late_dest = cur_tw->late_arr + work_time;
    } else {
      // wait time case
      cur_tw->early_arr = plan_late_arr;
      cur_tw->late_arr = plan_late_arr;
      cur_tw->early_dest = ori_tw->early + work_time;
      cur_tw->late_dest = ori_tw->early + work_time;
      cur_tw->wait_time = ori_tw->early - plan_late_arr;
    }
  } else {
    // over time case
    cur_tw->early_arr = plan_early_arr;
    cur_tw->late_arr = plan_early_arr;
    cur_tw->early_dest = plan_early_arr + work_time;
    cur_tw->late_dest = plan_early_arr + work_time;
    cur_tw->over_time = plan_early_arr - ori_tw->late;
  }

  return cur_tw;
}

void TimeWindowInfer::forward_infer_s_m(const TimeWindowPlan* pre_node_plan_tw, long cost_time,
                                        long work_time, std::vector<TimeWindow*>& ori_tws,
                                        std::vector<TimeWindowPlan*>& plan_node_tws) {
  bool non_wait_over_flag = false;
  if (plan_node_tws.size() > 1 ||
      (plan_node_tws.size() > 0 && plan_node_tws[0]->is_zero_wait_over_time())) {
    non_wait_over_flag = true;
  }

  long plan_early_arr = pre_node_plan_tw->early_dest + cost_time;
  long plan_late_arr = pre_node_plan_tw->late_dest + cost_time;

  for (const auto& cur_ori_tw : ori_tws) {
    if (plan_late_arr < cur_ori_tw->early) {
      // case: wait time
      if (non_wait_over_flag) {
        break;
      }
      if (plan_node_tws.size() == 0 ||
          (plan_node_tws[0]->over_time > 0 ||
           (cur_ori_tw->early - plan_late_arr) < plan_node_tws[0]->wait_time)) {
        for (auto& tw : plan_node_tws) {
          delete tw;
        }
        plan_node_tws.clear();
        plan_node_tws.push_back(
            forward_infer_s_s(pre_node_plan_tw, cost_time, work_time, cur_ori_tw));
      }
    } else if (plan_early_arr > cur_ori_tw->late) {
      // case: over time
      if (non_wait_over_flag) {
        continue;
      }
      if (plan_node_tws.size() == 0 ||
          (plan_node_tws[0]->wait_time == 0 &&
           (plan_early_arr - cur_ori_tw->late) < plan_node_tws[0]->over_time)) {
        for (auto& tw : plan_node_tws) {
          delete tw;
        }
        plan_node_tws.clear();
        plan_node_tws.push_back(
            forward_infer_s_s(pre_node_plan_tw, cost_time, work_time, cur_ori_tw));
      }
    } else {
      if (!non_wait_over_flag) {
        for (auto& tw : plan_node_tws) {
          delete tw;
        }
        plan_node_tws.clear();
        non_wait_over_flag = true;
      }
      plan_node_tws.push_back(
          forward_infer_s_s(pre_node_plan_tw, cost_time, work_time, cur_ori_tw));
    }
  }
}

std::vector<TimeWindowPlan*> TimeWindowInfer::forward_infer(
    std::vector<TimeWindowPlan*>& pre_node_plan_tws, long cost_time, long work_time,
    std::vector<TimeWindow*>& ori_tws) {
  std::vector<TimeWindowPlan*> plan_node_tws;
  for (auto& pre_node_plan_tw : pre_node_plan_tws) {
    forward_infer_s_m(pre_node_plan_tw, cost_time, work_time, ori_tws, plan_node_tws);
  }
  return plan_node_tws;
}

TimeWindow* TimeWindowUntils::copy_time_window(TimeWindow* tw) {
  return new TimeWindow(tw->early, tw->late);
}

TimeWindow* TimeWindowUntils::intersection(TimeWindow* tw_a, TimeWindow* tw_b) {
  const long tmp_early = std::max(tw_a->early, tw_b->early);
  const long tmp_late = std::min(tw_a->late, tw_b->late);
  if (tmp_early > tmp_late) {
    return nullptr;
  }
  return new TimeWindow(tmp_early, tmp_late);
}

std::vector<TimeWindow*> TimeWindowUntils::merge_time_windows(std::vector<TimeWindow*> tws) {
  std::vector<TimeWindow*> res_tws;
  for (auto& tw : tws) {
    if (res_tws.size() == 0) {
      res_tws.push_back(copy_time_window(tw));
    } else {
      auto last_tw = res_tws.back();
      if ((tw->early - last_tw->late) <=
          TimeWindowParameter::INTERVAL_SECS_CONTINUOUS_TIME_BUCKET) {
        res_tws.pop_back();
        res_tws.push_back(new TimeWindow(last_tw->early, tw->late));
        delete last_tw;
      } else {
        res_tws.push_back(copy_time_window(tw));
      }
    }
    delete tw;
  }

  return res_tws;
}
