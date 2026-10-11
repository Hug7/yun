/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#include "pdm_cache_time_window.h"

#include <ranges>
#include <vector>

#include "bdm_time_window.h"

// ====== implement of NodeTimeWindowKey ======
NodeTimeWindowKey::NodeTimeWindowKey(ActivityType activity_type, const Location* location)
    : activity_type(activity_type), location(location), orders() {
  this->order_len = 0;
  this->hash_code = tw_key_hash_code(activity_type, location->ind, nullptr, 0);
}

NodeTimeWindowKey::NodeTimeWindowKey(ActivityType activity_type, const Location* location,
                                     const std::vector<const Order*>& orders)
    : activity_type(activity_type), location(location), orders(orders) {
  this->order_len = static_cast<int>(orders.size());
  this->hash_code =
      tw_key_hash_code(activity_type, location->ind, orders.data(), this->order_len);
}

// ====== implement of NodeTimeWindowStoreBuffer ======
NodeTimeWindowStoreBuffer::~NodeTimeWindowStoreBuffer() {
  for (auto& tws : this->pick | std::views::values) {
    for (const auto tw : tws) {
      delete tw;
    }
  }
  this->pick.clear();

  for (auto& tws : this->drop | std::views::values) {
    for (const auto tw : tws) {
      delete tw;
    }
  }
  this->drop.clear();

  for (auto& val : this->other | std::views::values) {
    for (const auto tw : val) {
      delete tw;
    }
  }
  this->other.clear();
}

const std::vector<TimeWindow*>* NodeTimeWindowStoreBuffer::get_or_create_pick(
    const Node* node, const NodeTimeWindowKey& node_tw_key) {
  const auto it = this->pick.find(node_tw_key);
  if (it != this->pick.end()) {
    return &it->second;
  }
  auto tws = node->intersection_time_windows();
  const auto [new_it, _] = this->pick.emplace(node_tw_key, std::move(tws));
  return &new_it->second;
}

const std::vector<TimeWindow*>* NodeTimeWindowStoreBuffer::get_or_create_drop(
    const Node* node, const NodeTimeWindowKey& node_tw_key) {
  const auto it = this->drop.find(node_tw_key);
  if (it != this->drop.end()) {
    return &it->second;
  }
  auto tws = node->intersection_time_windows();
  const auto [new_it, _] = this->drop.emplace(node_tw_key, std::move(tws));
  return &new_it->second;
}

const std::vector<TimeWindow*>* NodeTimeWindowStoreBuffer::get_or_create_other(
    const NodeTimeWindowKey& node_tw_key, const PlanDatetimeRange* plan_datetime_range) {
  const auto it = this->other.find(node_tw_key);
  if (it != this->other.end()) {
    return &it->second;
  }
  std::vector<TimeWindow*> tws;
  tws.push_back(new TimeWindow(plan_datetime_range->start_time, plan_datetime_range->end_time));
  const auto [new_it, _] = this->other.emplace(node_tw_key, std::move(tws));
  return &new_it->second;
}

// ====== implement of NodeTimeWindowCache ======
NodeTimeWindowCache::NodeTimeWindowCache(const PlanDatetimeRange* plan_datetime_range)
    : plan_datetime_range(plan_datetime_range), pick(), drop(), other() {
  this->pick.reserve(CacheParameter::PICK_DROP_RESERVED_ENTRY_COUNT);
  this->drop.reserve(CacheParameter::PICK_DROP_RESERVED_ENTRY_COUNT);
}

NodeTimeWindowCache::~NodeTimeWindowCache() {
  // release pick cache
  for (auto& val : this->pick | std::views::values) {
    for (const auto tw : val) {
      delete tw;
    }
  }
  this->pick.clear();
  // release drop cache
  for (auto& val : this->drop | std::views::values) {
    for (const auto tw : val) {
      delete tw;
    }
  }
  this->drop.clear();

  for (auto& val : this->other | std::views::values) {
    for (const auto tw : val) {
      delete tw;
    }
  }
  this->other.clear();
}

const std::vector<TimeWindow*>* NodeTimeWindowCache::find_pick(
    const Node* node, std::vector<const Order*>& scratch) const {
  node->get_orders(scratch);
  const NodeTimeWindowKey key(node->activity_type, node->loc, scratch);
  const auto it = this->pick.find(key);
  if (it == this->pick.end()) {
    return nullptr;
  }
  return &it->second;
}

const std::vector<TimeWindow*>* NodeTimeWindowCache::find_drop(
    const Node* node, std::vector<const Order*>& scratch) const {
  node->get_orders(scratch);
  const NodeTimeWindowKey key(node->activity_type, node->loc, scratch);
  const auto it = this->drop.find(key);
  if (it == this->drop.end()) {
    return nullptr;
  }
  return &it->second;
}

const std::vector<TimeWindow*>* NodeTimeWindowCache::find_other(const Node* node) const {
  const NodeTimeWindowKey key(node->activity_type, node->loc);
  const auto it = this->other.find(key);
  if (it == this->other.end()) {
    return nullptr;
  }
  return &it->second;
}

const std::vector<TimeWindow*>* NodeTimeWindowCache::get_or_create_pick(
    const Node* node, const NodeTimeWindowKey& node_time_window_key) {
  if (this->is_parallel()) {
    throw std::logic_error(
        "NodeTimeWindowCache: 并行阶段禁止直写冻结表, 必须先挂载 NodeTimeWindowStoreBuffer");
  }

  const auto it = this->pick.find(node_time_window_key);
  if (it != this->pick.end()) {
    return &it->second;
  }
  auto tws = node->intersection_time_windows();
  const auto [new_it, _] = this->pick.emplace(node_time_window_key, std::move(tws));
  return &new_it->second;
}

const std::vector<TimeWindow*>* NodeTimeWindowCache::get_or_create_drop(
    const Node* node, const NodeTimeWindowKey& node_time_window_key) {
  if (this->is_parallel()) {
    throw std::logic_error(
        "NodeTimeWindowCache: 并行阶段禁止直写冻结表, 必须先挂载 NodeTimeWindowStoreBuffer");
  }

  const auto it = this->drop.find(node_time_window_key);
  if (it != this->drop.end()) {
    return &it->second;
  }
  auto tws = node->intersection_time_windows();
  const auto [new_it, _] = this->drop.emplace(node_time_window_key, std::move(tws));
  return &new_it->second;
}

const std::vector<TimeWindow*>* NodeTimeWindowCache::get_or_create_other(const NodeTimeWindowKey& node_tw_key) {
  if (this->is_parallel()) {
    throw std::logic_error(
        "NodeTimeWindowCache: 并行阶段禁止直写冻结表, 必须先挂载 NodeTimeWindowStoreBuffer");
  }
  const auto it = this->other.find(node_tw_key);
  if (it != this->other.end()) {
    return &it->second;
  }
  std::vector<TimeWindow*> tws;
  tws.push_back(
      new TimeWindow(this->plan_datetime_range->start_time, this->plan_datetime_range->end_time));
  const auto [new_it, _] = this->other.emplace(node_tw_key, std::move(tws));
  return &new_it->second;
}

void NodeTimeWindowCache::archive(NodeTimeWindowStoreBuffer* node_tw_store_buffer) {
  if (this->is_parallel()) {
    throw std::logic_error("NodeTimeWindowCache: 归档必须在并行屏障之后的串行阶段执行");
  }
  for (auto& [key, tws] : node_tw_store_buffer->pick) {
    if (this->pick.contains(key)) {
      // 重复键: 冻结表已持有等价条目,写缓冲这一份必须显式释放
      for (const auto tw : tws) {
        delete tw;
      }
      continue;
    }
    this->pick.emplace(key, std::move(tws));
  }
  for (auto& [key, tws] : node_tw_store_buffer->drop) {
    if (this->drop.contains(key)) {
      // 重复键: 冻结表已持有等价条目,写缓冲这一份必须显式释放
      for (const auto tw : tws) {
        delete tw;
      }
      continue;
    }
    this->drop.emplace(key, std::move(tws));
  }
  for (auto& [key, tws] : node_tw_store_buffer->other) {
    if (this->other.contains(key)) {
      for (const auto tw : tws) {
        delete tw;
      }
      continue;
    }
    this->other.emplace(key, std::move(tws));
  }
  // 条目所有权已转移完毕, 只清容器;写缓冲可继续复用
  node_tw_store_buffer->pick.clear();
  node_tw_store_buffer->drop.clear();
  node_tw_store_buffer->other.clear();
}

const std::vector<TimeWindow*>& NodeTimeWindowCache::lookup(
    NodeTimeWindowStoreBuffer* node_tw_store_buffer, const Node* node) {
  const bool exist_store_buffer = node_tw_store_buffer != nullptr;

  if (node->is_drop()) {
    // 冻结表只读查找, 命中率约 92%, 命中路径零写入零分配
    std::vector<const Order*>& scratch =
        exist_store_buffer ? node_tw_store_buffer->orders_scratch : this->serial_scratch;
    // 填充订单
    node->get_orders(scratch);
    // 冻结表查找走非持有视图, 命中路径零堆分配
    const NodeTimeWindowKeyView node_tw_view(node->activity_type, node->loc, scratch);

    if (exist_store_buffer) {
      // 先查主表的cache
      const auto it = this->drop.find(node_tw_view);
      if (it != this->drop.end()) {
        return it->second;
      }
      // 查缓冲区表的cache，不存在时则直接创建(未命中才构造持有键)
      return *node_tw_store_buffer->get_or_create_drop(
          node, NodeTimeWindowKey(node->activity_type, node->loc, scratch));
    }
    return *this->get_or_create_drop(
        node, NodeTimeWindowKey(node->activity_type, node->loc, scratch));
  } else if (node->is_pick()) {
    // 冻结表只读查找, 命中率约 92%, 命中路径零写入零分配
    std::vector<const Order*>& scratch =
        exist_store_buffer ? node_tw_store_buffer->orders_scratch : this->serial_scratch;
    // 填充订单
    node->get_orders(scratch);
    // 冻结表查找走非持有视图, 命中路径零堆分配
    const NodeTimeWindowKeyView node_tw_view(node->activity_type, node->loc, scratch);

    if (exist_store_buffer) {
      // 先查主表的cache
      const auto it = this->pick.find(node_tw_view);
      if (it != this->pick.end()) {
        return it->second;
      }
      // 查缓冲区表的cache，不存在时则直接创建(未命中才构造持有键)
      return *node_tw_store_buffer->get_or_create_pick(
          node, NodeTimeWindowKey(node->activity_type, node->loc, scratch));
    }
    return *this->get_or_create_pick(
        node, NodeTimeWindowKey(node->activity_type, node->loc, scratch));
  } else {
    // 构造key
    const NodeTimeWindowKey node_tw_key(node->activity_type, node->loc);
    if (exist_store_buffer) {
      // 先查主表的cache
      const auto it = this->other.find(node_tw_key);
      if (it != this->other.end()) {
        return it->second;
      }
      // 查缓冲区表的cache，不存在时则直接创建
      const auto* buffer_tws = node_tw_store_buffer->get_or_create_other(node_tw_key, this->plan_datetime_range);
      return *buffer_tws;
    }
    return *this->get_or_create_other(node_tw_key);
  }
}
