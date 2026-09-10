/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#include "pdm_time_window_cache.h"
#include <vector>
#include "bdm_time_window.h"

// ====== implement of NodeTimeWindowKey ======
NodeTimeWindowKey::NodeTimeWindowKey(ActivityType activity_type, const Location* location)
    : activity_type(activity_type), location(location), orders() {
  this->order_len = 0;
  this->hash_code = static_cast<int>(activity_type) + 1;
  this->hash_code = this->hash_code * 31 + location->ind;
}

NodeTimeWindowKey::NodeTimeWindowKey(ActivityType activity_type, const Location* location,
                                     const std::vector<const Order*>& orders)
    : activity_type(activity_type), location(location), orders(orders) {
  this->order_len = static_cast<int>(orders.size());
  this->hash_code = static_cast<int>(activity_type) + 1;
  this->hash_code = this->hash_code * 31 + location->ind;
  for (int u = 0; u < order_len; u++) {
    this->hash_code = this->hash_code * 31 + orders[u]->ind;
  }
}

// ====== implement of NodeTimeWindowCache ======
NodeTimeWindowCache::~NodeTimeWindowCache() {
  for (auto& it : this->pick_drop_cache) {
    for (auto& tws : it.second) {
      delete tws;
    }
  }
  this->pick_drop_cache.clear();
  for (auto& it : this->other_cache) {
    for (auto& tws : it.second) {
      delete tws;
    }
  }
  this->other_cache.clear();
}

std::vector<TimeWindow*> NodeTimeWindowCache::get_pick_drop_time_windows(const Node* node) {
  if (!(node->is_pick() || node->is_drop())) {
    return this->get_other_time_windows(node);
  }
  NodeTimeWindowKey key(node->activity_type, node->loc, node->get_orders());
  auto it = this->pick_drop_cache.find(key);
  if (it != this->pick_drop_cache.end()) {
    return it->second;
  } else {
    auto tws = node->intersection_time_windows();
    this->pick_drop_cache.emplace(std::move(key), tws);
    return tws;
  }
}

std::vector<TimeWindow*> NodeTimeWindowCache::get_other_time_windows(const Node* node) {
  NodeTimeWindowKey key(node->activity_type, node->loc);
  auto it = this->other_cache.find(key);
  if (it != this->other_cache.end()) {
    return it->second;
  } else {
    // TODO 默认使用规划时间段作为空时间窗, 后续需要考虑取规划和动作类型的交集
    std::vector<TimeWindow*> tws;
    tws.push_back(new TimeWindow(this->plan_datetime_range->start_time, this->plan_datetime_range->end_time));
    this->other_cache.emplace(std::move(key), tws);
    return tws;
  }
}
