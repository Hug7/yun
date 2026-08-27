/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#include "bd_time_window_utils.h"

#include <algorithm>
#include <memory>

#include "bd_time_window.h"
#include "bd_time_window_plan.h"

// ====== implement of TimeWindowInfer ======
TimeWindowPlan::UPtr TimeWindowInfer::forward_s_s(TimeWindowPlan::UPtr& pre_node_plan_tw,
                                                  const long cost_time, const long work_time,
                                                  const TimeWindow* ori_tw) {
  TimeWindowPlan::UPtr cur_tw = std::make_unique<TimeWindowPlan>();

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

void TimeWindowInfer::forward_s_m(TimeWindowPlan::UPtr& pre_node_plan_tw, const long cost_time,
                                  const long work_time, std::vector<TimeWindow*>& ori_tws,
                                  TimeWindowPlan::VecUPtr& plan_node_tws) {
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
        plan_node_tws.clear();
        plan_node_tws.push_back(
            forward_s_s(pre_node_plan_tw, cost_time, work_time, cur_ori_tw));
      }
    } else if (plan_early_arr > cur_ori_tw->late) {
      // case: over time
      if (non_wait_over_flag) {
        continue;
      }
      if (plan_node_tws.size() == 0 ||
          (plan_node_tws[0]->wait_time == 0 &&
           (plan_early_arr - cur_ori_tw->late) < plan_node_tws[0]->over_time)) {
        plan_node_tws.clear();
        plan_node_tws.push_back(
            forward_s_s(pre_node_plan_tw, cost_time, work_time, cur_ori_tw));
      }
    } else {
      if (!non_wait_over_flag) {
        plan_node_tws.clear();
        non_wait_over_flag = true;
      }
      plan_node_tws.push_back(
          forward_s_s(pre_node_plan_tw, cost_time, work_time, cur_ori_tw));
    }
  }
}

TimeWindowPlan::VecUPtr TimeWindowInfer::forward(TimeWindowPlan::VecUPtr& pre_node_plan_tws,
                                                 const long cost_time, const long work_time,
                                                 std::vector<TimeWindow*>& ori_tws) {
  TimeWindowPlan::VecUPtr plan_node_tws;
  for (auto& pre_node_plan_tw : pre_node_plan_tws) {
    forward_s_m(pre_node_plan_tw, cost_time, work_time, ori_tws, plan_node_tws);
  }
  return plan_node_tws;
}

TimeWindow* TimeWindowUntils::intersection(const TimeWindow* tw_a, const TimeWindow* tw_b) {
  const long tmp_early = std::max(tw_a->early, tw_b->early);
  const long tmp_late = std::min(tw_a->late, tw_b->late);
  if (tmp_early > tmp_late) {
    return nullptr;
  }
  return new TimeWindow(tmp_early, tmp_late);
}

std::vector<TimeWindow*> TimeWindowUntils::intersection_tws_arr(
    std::vector<std::vector<TimeWindow*>>& tws_arr) {
  if (tws_arr.empty()) {
    return {};
  }
  // 时间窗集合大小
  const std::size_t n = tws_arr.size();
  // 每个向量当前的读取位置
  std::vector<std::size_t> idx(n, 0);
  // 时间窗交集结果
  std::vector<TimeWindow*> res;

  while (true) {
    // 获取最大最小时间窗
    long cur_max_early = tws_arr[0][idx[0]]->early;
    long cur_min_late = tws_arr[0][idx[0]]->late;
    long cur_min_late_ind = 0;
    for (std::size_t u = 1; u < n; ++u) {
      TimeWindow* cand = tws_arr[u][idx[u]];
      cur_max_early = std::max(cur_max_early, cand->early);
      if (cur_min_late > cand->late) {
        cur_min_late = cand->late;
        cur_min_late_ind = u;
      }
    }
    // 记录交集时间窗
    if (cur_min_late > cur_max_early) {
      res.push_back(new TimeWindow(cur_max_early, cur_min_late));
    }
    // 推进选中的向量
    if (++idx[cur_min_late_ind] >= tws_arr[cur_min_late_ind].size()) {
      break;
    }
  }

  return res;
}

std::vector<TimeWindow*> TimeWindowUntils::merge_time_windows(std::vector<TimeWindow*>& tws) {
  std::vector<TimeWindow*> res_tws;
  for (auto& tw : tws) {
    if (res_tws.size() == 0) {
      res_tws.push_back(new TimeWindow(tw));
    } else {
      auto last_tw = res_tws.back();
      if ((tw->early - last_tw->late) <=
          TimeWindowParameter::INTERVAL_SECS_CONTINUOUS_TIME_BUCKET) {
        res_tws.pop_back();
        res_tws.push_back(new TimeWindow(last_tw->early, tw->late));
        delete last_tw;
      } else {
        res_tws.push_back(new TimeWindow(tw));
      }
    }
    delete tw;
  }

  return res_tws;
}
