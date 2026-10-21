/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#include "algo_alns_ruin_random_removal_order.h"

// ====== implement of AlnsRuinRandomRemovalOrder ======
AlnsRuinRandomRemovalOrder::AlnsRuinRandomRemovalOrder(const SolverContext* _context,
                                                       const sol::table& cfg)
    : AlnsRuinOperator(AlnsRuinOperatorCode::RuinRandomRemovalOrder, _context) {
  if (cfg[AlnsRuinRandomRemovalOrderLuaField::SELECT_ORDER_MIN_RATE].valid()) {
    this->select_order_min_rate =
        static_cast<double>(cfg[AlnsRuinRandomRemovalOrderLuaField::SELECT_ORDER_MIN_RATE]);
  }
  if (cfg[AlnsRuinRandomRemovalOrderLuaField::SELECT_ORDER_MAX_RATE].valid()) {
    this->select_order_max_rate =
        static_cast<double>(cfg[AlnsRuinRandomRemovalOrderLuaField::SELECT_ORDER_MAX_RATE]);
  }
  if (cfg[AlnsRuinRandomRemovalOrderLuaField::SELECT_ORDER_RANDOM_SEED].valid()) {
    this->select_order_random_seed =
        static_cast<int>(cfg[AlnsRuinRandomRemovalOrderLuaField::SELECT_ORDER_RANDOM_SEED]);
  }
  this->select_order_random = std::make_unique<RandomUtils>(this->select_order_random_seed);

  // 校验参数
  if (this->select_order_min_rate < 0.0 || this->select_order_min_rate > 1.0) {
    throw std::invalid_argument(this->code + ": `select_order_min_rate` must be in [0, 1], got " +
                                std::to_string(this->select_order_min_rate));
  }

  if (this->select_order_max_rate < 0.0 || this->select_order_max_rate > 1.0) {
    throw std::runtime_error(this->code + ": `select_order_max_rate` must be in [0, 1], got " +
                             std::to_string(this->select_order_max_rate));
  }

  if (this->select_order_min_rate > this->select_order_max_rate) {
    throw std::runtime_error(this->code + ": `select_order_min_rate` (" +
                             std::to_string(this->select_order_min_rate) +
                             ") must not exceed select_order_max_rate (" +
                             std::to_string(this->select_order_max_rate) + ")");
  }
}

std::unordered_map<int, AlnsRuinLoadActivity::UPtr> AlnsRuinRandomRemovalOrder::execute(
    Solution* sol) {
  std::unordered_map<int, AlnsRuinLoadActivity::UPtr> select_order_res;
  // 遍历解中所有的activity, 收集每个订单的提货activity: (所属load索引, pick activity)
  std::vector<std::pair<int, Activity*>> pick_activities;
  pick_activities.reserve(sol->orders_view.size());
  const int load_count = static_cast<int>(sol->loads.size());
  for (int load_ind = 0; load_ind < load_count; ++load_ind) {
    for (const auto& activity : sol->loads[load_ind]->get_pick_activities()) {
      pick_activities.emplace_back(load_ind, activity);
    }
  }
  // 没有要处理的订单
  const int order_count = static_cast<int>(pick_activities.size());
  if (order_count == 0) {
    return select_order_res;
  }
  // 随机确定要移除的订单数
  const int select_order_count =
      std::max(1, this->select_order_random->random_int(order_count, this->select_order_min_rate,
                                                        this->select_order_max_rate));
  // 随机打乱后取前 select_order_count 个订单, 按所属load归集待移除的activity
  this->select_order_random->shuffle(pick_activities);
  for (int u = 0; u < select_order_count; ++u) {
    const auto [load_ind, pick_activity] = pick_activities[u];
    auto& load_activities = select_order_res[load_ind];
    if (load_activities == nullptr) {
      load_activities = std::make_unique<AlnsRuinLoadActivity>(load_ind);
    }
    // 提卸成对移除
    load_activities->activities.emplace_back(pick_activity);
    load_activities->activities.emplace_back(pick_activity->related);
  }

  return select_order_res;
}
