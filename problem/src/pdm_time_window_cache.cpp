/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#include "pdm_time_window_cache.h"

#include <algorithm>
#include <ranges>
#include <vector>

#include "bdm_time_window.h"

// ====== implement of NodeTimeWindowKey ======
NodeTimeWindowKey::NodeTimeWindowKey(ActivityType activity_type, const Location* location)
    : activity_type(activity_type), location(location), orders(nullptr) {
  this->order_len = 0;
  this->hash_code = static_cast<int>(activity_type) + 1;
  this->hash_code = this->hash_code * 31 + location->ind;
}

NodeTimeWindowKey::NodeTimeWindowKey(ActivityType activity_type, const Location* location,
                                     const std::vector<const Order*>& orders)
    : activity_type(activity_type), location(location), orders(orders.data()) {
  this->order_len = static_cast<int>(orders.size());
  this->hash_code = static_cast<int>(activity_type) + 1;
  this->hash_code = this->hash_code * 31 + location->ind;
  for (int u = 0; u < order_len; u++) {
    this->hash_code = this->hash_code * 31 + orders[u]->ind;
  }
}

// ====== implement of NodeTimeWindowCache ======
NodeTimeWindowCache::~NodeTimeWindowCache() {
  for (auto& val : this->pick_drop_cache | std::views::values) {
    for (const auto& tws : val.tws) {
      delete tws;
    }
  }
  this->pick_drop_cache.clear();
  for (auto& val : this->other_cache | std::views::values) {
    for (const auto& tws : val) {
      delete tws;
    }
  }
  this->other_cache.clear();
}

int NodeTimeWindowCache::acquire_orders_slot() {
  if (!this->orders_arena_free_slots.empty()) {
    const int slot = this->orders_arena_free_slots.back();
    this->orders_arena_free_slots.pop_back();
    return slot;
  }
  this->orders_arena.emplace_back();
  return static_cast<int>(this->orders_arena.size()) - 1;
}

NodeTimeWindowCache::PickDropMap::iterator NodeTimeWindowCache::erase_pick_drop(
    NodeTimeWindowCache::PickDropMap::iterator it) {
  for (const auto tw : it->second.tws) {
    delete tw;
  }
  const int slot = it->second.arena_slot;
  this->orders_arena[slot].clear();
  this->orders_arena_free_slots.push_back(slot);
  return this->pick_drop_cache.erase(it);
}

void NodeTimeWindowCache::evict_pick_drop_overflow() {
  const int cur_size = static_cast<int>(this->pick_drop_cache.size());
  // <= 0 的容量表示不限制; 一次只淘汰刚超出的部分会让扫描过于频繁, 故多清出 1/8 容量
  if (this->cache_capacity <= 0 || cur_size <= this->cache_capacity) {
    return;
  }
  const int keep = this->cache_capacity - this->cache_capacity / 8;
  std::vector<long long> ticks;
  ticks.reserve(cur_size);
  for (const auto& val : this->pick_drop_cache | std::views::values) {
    ticks.push_back(val.tick);
  }
  const int deadline_ind = cur_size - keep - 1;
  std::ranges::nth_element(ticks, ticks.begin() + deadline_ind);
  const long long deadline = ticks[deadline_ind];
  for (auto it = this->pick_drop_cache.begin(); it != this->pick_drop_cache.end();) {
    if (it->second.tick <= deadline) {
      it = this->erase_pick_drop(it);
    } else {
      ++it;
    }
  }
}

std::vector<TimeWindow*> NodeTimeWindowCache::get_pick_drop_time_windows(const Node* node) {
  if (!(node->is_pick() || node->is_drop())) {
    return this->get_other_time_windows(node);
  }
  node->get_orders(this->orders_scratch);
  NodeTimeWindowKey key(node->activity_type, node->loc, this->orders_scratch);
  auto it = this->pick_drop_cache.find(key);
  if (it != this->pick_drop_cache.end()) {
    it->second.tick = ++this->access_clock;
    return it->second.tws;
  }
  auto tws = node->intersection_time_windows();
  // 复用缓冲会被下一次查找覆盖, 入库前先把 orders 落到 arena 上
  const int slot = this->acquire_orders_slot();
  this->orders_arena[slot].assign(this->orders_scratch.begin(), this->orders_scratch.end());
  key.orders = this->orders_arena[slot].data();
  this->pick_drop_cache.emplace(key, PickDropEntry{tws, slot, ++this->access_clock});
  this->evict_pick_drop_overflow();
  return tws;
}

std::vector<TimeWindow*> NodeTimeWindowCache::get_other_time_windows(const Node* node) {
  NodeTimeWindowKey key(node->activity_type, node->loc);
  const auto it = this->other_cache.find(key);
  if (it != this->other_cache.end()) {
    return it->second;
  }
  std::vector<TimeWindow*> tws;
  tws.push_back(
      new TimeWindow(this->plan_datetime_range->start_time, this->plan_datetime_range->end_time));
  this->other_cache.emplace(key, tws);
  return tws;
}
