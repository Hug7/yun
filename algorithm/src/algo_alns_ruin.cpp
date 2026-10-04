/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#include "algo_alns_ruin.h"

#include <ranges>
#include <vector>

// ====== implement of AlnsRuinOperator ======
void AlnsRuinOperator::call(Solution* sol) {
  // 执行算子获取要移除的 load->activities 列表
  auto ruin_load_activities_map = this->execute(sol);

  // 对待移除的 load->activities 按照 load 索引倒序排序(为了方便移除整个load)
  std::vector<AlnsRuinLoadActivity::UPtr> ruin_load_activities_vec;
  ruin_load_activities_vec.reserve(ruin_load_activities_map.size());
  for (auto& load_activities : ruin_load_activities_map | std::views::values) {
    ruin_load_activities_vec.emplace_back(std::move(load_activities));
  }
  std::ranges::sort(ruin_load_activities_vec.begin(), ruin_load_activities_vec.end(),
                    [](const auto& lhs, const auto& rhs) { return lhs->load_ind > rhs->load_ind; });

  // 从 solution 中移除指定的 load->activities
  for (const auto& load_activities : ruin_load_activities_vec) {
    const int cur_activity_len = static_cast<int>(load_activities->activities.size());
    if (cur_activity_len == 0) {
      // activities 为空则认为移除整个load
      sol->remove_load_by_ind(load_activities->load_ind);
      continue;
    }
    // 部分移除: activities 非空
    Load* cur_load = sol->loads[load_activities->load_ind];
    // 预判断：若 activities 覆盖了该 load 的全部 activity，等价于整 load 删除
    if (cur_load->get_activity_count() == cur_activity_len) {
      sol->remove_load_by_ind(load_activities->load_ind);
      continue;
    }
    // 移除的 activities 放回 solution->unassigned_orders
    sol->add_unassigned_orders_by_activities(load_activities->activities);

    // 将 activities 从 cur_load 中移除(空 node 同步移除)
    cur_load->remove_activities(load_activities->activities);
    // 摘空后只剩首尾哨兵: 退化为整 load 删除(顺带释放车辆)
    if (cur_load->is_empty()) {
      sol->remove_load_by_ind(load_activities->load_ind);
      continue;
    }
    // 重置 cur_load 的 route 属性
    cur_load->reset_route_profile();
    // 重新评价 cur_load 的约束项
    this->context->problem->eval_load(cur_load);
    // 再次判断是否满足约束。 如果不满足约束则将 cur_load 中 activities 从后往前移除直到满足约束为止
    while (cur_load->is_infeasible()) {
      auto cur_node = cur_load->last_node->prev;
      // 将待移除 last activity 对应的 order 放入 solution 的待指派订单池
      sol->add_unassigned_order(cur_node->last->order);
      // 从 cur_load 中移除 cur_node 的 last activity 和 related of last activity
      cur_load->remove_activities({cur_node->last, cur_node->last->related});
      if (cur_load->is_empty()) {
        sol->remove_load_by_ind(load_activities->load_ind);
        break;
      }
      // 重置 cur_load 的 route 属性
      cur_load->reset_route_profile();
      // 重新评价 cur_load 的约束项
      this->context->problem->eval_load(cur_load);
    }
  }
  // todo 重新分配车辆资源，保证最优车型

}
