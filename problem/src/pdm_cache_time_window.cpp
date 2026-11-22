/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#include "pdm_cache_time_window.h"

#include <algorithm>
#include <stdexcept>
#include <utility>
#include <vector>

#include "bdm_time_window.h"
#include "c_constant.h"

namespace {
/**
 * @brief 把节点上的订单直接映射成时间窗模式 id, 并整理成缓存键的规范形式
 * @details 缓存值只取决于这些时间窗内容的集合, 因此键里不需要订单身份、订单顺序或站点;
 * 排序去重后, 同一批内容的不同订单组合会落到同一个键上。
 */
void collect_pattern_ids(const Node* node, bool is_pick, std::vector<int>& pattern_ids) {
  node->get_time_window_pattern_ids(is_pick, pattern_ids);
  tw_key_canonicalize(pattern_ids);
}
}  // namespace

// ====== implement of NodeTimeWindowKey ======
NodeTimeWindowKey::NodeTimeWindowKey(std::vector<int> pattern_ids)
    : pattern_ids(std::move(pattern_ids)),
      pattern_len(static_cast<int>(this->pattern_ids.size())),
      hash_code(tw_key_hash_code(this->pattern_ids.data(), this->pattern_len)) {}

// ====== implement of NodeTimeWindowStoreBuffer ======
const std::vector<TimeWindow>* NodeTimeWindowStoreBuffer::get_or_create_pick(
    const Node* node, const NodeTimeWindowKey& node_tw_key) {
  const auto it = this->pick.find(node_tw_key);
  if (it != this->pick.end()) {
    return &it->second;
  }
  auto tws = node->intersection_time_windows();
  const auto [new_it, _] = this->pick.emplace(node_tw_key, std::move(tws));
  return &new_it->second;
}

const std::vector<TimeWindow>* NodeTimeWindowStoreBuffer::get_or_create_drop(
    const Node* node, const NodeTimeWindowKey& node_tw_key) {
  const auto it = this->drop.find(node_tw_key);
  if (it != this->drop.end()) {
    return &it->second;
  }
  auto tws = node->intersection_time_windows();
  const auto [new_it, _] = this->drop.emplace(node_tw_key, std::move(tws));
  return &new_it->second;
}

const std::vector<TimeWindow>* NodeTimeWindowStoreBuffer::get_or_create_other(
    const NodeTimeWindowKey& node_tw_key, const PlanDatetimeRange* plan_datetime_range) {
  const auto it = this->other.find(node_tw_key);
  if (it != this->other.end()) {
    return &it->second;
  }
  std::vector<TimeWindow> tws{
      TimeWindow(plan_datetime_range->start_time, plan_datetime_range->end_time)};
  const auto [new_it, _] = this->other.emplace(node_tw_key, std::move(tws));
  return &new_it->second;
}

// ====== implement of NodeTimeWindowCache ======
NodeTimeWindowCache::NodeTimeWindowCache(const PlanDatetimeRange* plan_datetime_range)
    : plan_datetime_range(plan_datetime_range), pick(), drop(), other() {
  this->pick.reserve(CacheParameter::PICK_DROP_RESERVED_ENTRY_COUNT);
  this->drop.reserve(CacheParameter::PICK_DROP_RESERVED_ENTRY_COUNT);
}

void NodeTimeWindowCache::recycle_if_full(
    std::unordered_map<NodeTimeWindowKey, std::vector<TimeWindow>>& table) {
  const auto max_entries = static_cast<std::size_t>(CacheParameter::PICK_DROP_MAX_ENTRY_COUNT);
  if (table.size() < max_entries) {
    return;
  }
  // 只淘汰到水位线(约 3/4): 按桶序等间隔删除, 统计上等价于随机淘汰, 热点条目大概率保留;
  // 整表清空会让所有热点同时失效, 把重算集中到清空后的那一小段。读路径仍不写本表,
  // 因此并行阶段无需任何额外的同步或访问计数。
  const auto target = static_cast<std::size_t>(CacheParameter::PICK_DROP_RECYCLE_TARGET_COUNT);
  std::size_t remaining = table.size() - target;
  const std::size_t step = std::max<std::size_t>(1, table.size() / remaining);
  std::size_t index = 0;
  for (auto it = table.begin(); it != table.end() && remaining > 0;) {
    if (index++ % step == 0) {
      it = table.erase(it);
      --remaining;
    } else {
      ++it;
    }
  }
}

const std::vector<TimeWindow>* NodeTimeWindowCache::find_pick(
    const Node* node, std::vector<int>& patterns_scratch) const {
  collect_pattern_ids(node, true, patterns_scratch);
  const NodeTimeWindowKeyView key(patterns_scratch);
  const auto it = this->pick.find(key);
  if (it == this->pick.end()) {
    return nullptr;
  }
  return &it->second;
}

const std::vector<TimeWindow>* NodeTimeWindowCache::find_drop(
    const Node* node, std::vector<int>& patterns_scratch) const {
  collect_pattern_ids(node, false, patterns_scratch);
  const NodeTimeWindowKeyView key(patterns_scratch);
  const auto it = this->drop.find(key);
  if (it == this->drop.end()) {
    return nullptr;
  }
  return &it->second;
}

const std::vector<TimeWindow>* NodeTimeWindowCache::find_other() const {
  const NodeTimeWindowKey key;
  const auto it = this->other.find(key);
  if (it == this->other.end()) {
    return nullptr;
  }
  return &it->second;
}

const std::vector<TimeWindow>* NodeTimeWindowCache::get_or_create_pick(
    const Node* node, const NodeTimeWindowKey& node_time_window_key) {
  if (this->is_parallel()) {
    throw std::logic_error(
        "NodeTimeWindowCache: 并行阶段禁止直写冻结表, 必须先挂载 NodeTimeWindowStoreBuffer");
  }

  const auto it = this->pick.find(node_time_window_key);
  if (it != this->pick.end()) {
    return &it->second;
  }
  recycle_if_full(this->pick);
  auto tws = node->intersection_time_windows();
  const auto [new_it, _] = this->pick.emplace(node_time_window_key, std::move(tws));
  return &new_it->second;
}

const std::vector<TimeWindow>* NodeTimeWindowCache::get_or_create_drop(
    const Node* node, const NodeTimeWindowKey& node_time_window_key) {
  if (this->is_parallel()) {
    throw std::logic_error(
        "NodeTimeWindowCache: 并行阶段禁止直写冻结表, 必须先挂载 NodeTimeWindowStoreBuffer");
  }

  const auto it = this->drop.find(node_time_window_key);
  if (it != this->drop.end()) {
    return &it->second;
  }
  recycle_if_full(this->drop);
  auto tws = node->intersection_time_windows();
  const auto [new_it, _] = this->drop.emplace(node_time_window_key, std::move(tws));
  return &new_it->second;
}

const std::vector<TimeWindow>* NodeTimeWindowCache::get_or_create_other(
    const NodeTimeWindowKey& node_tw_key) {
  if (this->is_parallel()) {
    throw std::logic_error(
        "NodeTimeWindowCache: 并行阶段禁止直写冻结表, 必须先挂载 NodeTimeWindowStoreBuffer");
  }
  const auto it = this->other.find(node_tw_key);
  if (it != this->other.end()) {
    return &it->second;
  }
  std::vector<TimeWindow> tws{
      TimeWindow(this->plan_datetime_range->start_time, this->plan_datetime_range->end_time)};
  const auto [new_it, _] = this->other.emplace(node_tw_key, std::move(tws));
  return &new_it->second;
}

void NodeTimeWindowCache::archive(NodeTimeWindowStoreBuffer* node_tw_store_buffer) {
  if (this->is_parallel()) {
    throw std::logic_error("NodeTimeWindowCache: 归档必须在并行屏障之后的串行阶段执行");
  }
  recycle_if_full(this->pick);
  for (auto& [key, tws] : node_tw_store_buffer->pick) {
    if (this->pick.contains(key)) {
      // 重复键: 冻结表已持有等价条目, 值语义下直接丢弃写缓冲这一份
      continue;
    }
    this->pick.emplace(key, std::move(tws));
  }
  recycle_if_full(this->drop);
  for (auto& [key, tws] : node_tw_store_buffer->drop) {
    if (this->drop.contains(key)) {
      // 重复键: 冻结表已持有等价条目, 值语义下直接丢弃写缓冲这一份
      continue;
    }
    this->drop.emplace(key, std::move(tws));
  }
  for (auto& [key, tws] : node_tw_store_buffer->other) {
    if (this->other.contains(key)) {
      continue;
    }
    this->other.emplace(key, std::move(tws));
  }
  // 条目已转移完毕, 只清容器;写缓冲可继续复用
  node_tw_store_buffer->pick.clear();
  node_tw_store_buffer->drop.clear();
  node_tw_store_buffer->other.clear();
}

const std::vector<TimeWindow>& NodeTimeWindowCache::lookup(
    NodeTimeWindowStoreBuffer* node_tw_store_buffer, const Node* node) {
  // other 节点的值恒定, 整表至多一条, 键为空
  if (!node->is_pick() && !node->is_drop()) {
    const NodeTimeWindowKey node_tw_key;
    if (node_tw_store_buffer != nullptr) {
      const auto it = this->other.find(node_tw_key);
      if (it != this->other.end()) {
        return it->second;
      }
      return *node_tw_store_buffer->get_or_create_other(node_tw_key, this->plan_datetime_range);
    }
    return *this->get_or_create_other(node_tw_key);
  }

  const bool is_pick = node->is_pick();
  const bool exist_store_buffer = node_tw_store_buffer != nullptr;
  std::vector<int>& patterns_scratch =
      exist_store_buffer ? node_tw_store_buffer->patterns_scratch : this->serial_patterns_scratch;

  // 直接收集模式 id, 排序去重后才是缓存键
  collect_pattern_ids(node, is_pick, patterns_scratch);

  // 冻结表只读查找走非持有视图, 命中路径零堆分配
  const NodeTimeWindowKeyView node_tw_view(patterns_scratch);
  auto& table = is_pick ? this->pick : this->drop;
  if (const auto it = table.find(node_tw_view); it != table.end()) {
    return it->second;
  }

  // 未命中才构造持有键, 并写入写缓冲(并行阶段)或直接入库(串行阶段)
  const NodeTimeWindowKey node_tw_key(patterns_scratch);
  if (exist_store_buffer) {
    return is_pick ? *node_tw_store_buffer->get_or_create_pick(node, node_tw_key)
                   : *node_tw_store_buffer->get_or_create_drop(node, node_tw_key);
  }
  return is_pick ? *this->get_or_create_pick(node, node_tw_key)
                 : *this->get_or_create_drop(node, node_tw_key);
}
