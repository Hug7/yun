/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <unordered_map>
#include <vector>

#include "bdm_location.h"
#include "bdm_time_window.h"
#include "c_constant.h"
#include "pdm_node.h"
#include "pdm_order.h"
#include "pdm_parameter.h"

/**
 * @brief 节点时间窗键
 * @details orders 是非持有视图: 查找时指向缓存的复用缓冲, 入库后指向缓存的 orders_arena。
 * 视图指向的内存在缓存析构前始终有效, 因此查找路径零堆分配。
 */
class NodeTimeWindowKey {
 public:
  const ActivityType activity_type;

  const Location* location;

  const Order* const* orders;

  int order_len;

  int hash_code;

  NodeTimeWindowKey(ActivityType activity_type, const Location* location);

  NodeTimeWindowKey(ActivityType activity_type, const Location* location,
                    const std::vector<const Order*>& orders);

  bool operator==(const NodeTimeWindowKey& other) const {
    if (this->hash_code != other.hash_code) {
      return false;
    }
    if (this->activity_type != other.activity_type) {
      return false;
    }
    if (this->location->ind != other.location->ind) {
      return false;
    }
    if (this->order_len != other.order_len) {
      return false;
    }
    for (int u = 0; u < this->order_len; u++) {
      if (this->orders[u]->ind != other.orders[u]->ind) {
        return false;
      }
    }
    return true;
  }
};

namespace std {
template <>
struct hash<NodeTimeWindowKey> {
  size_t operator()(const NodeTimeWindowKey& v) const { return std::hash<int>{}(v.hash_code); }
};
}  // namespace std

/**
 * @brief 时间窗缓存
 * @details 每个工作线程持有一份(由 LoadContext 按线程分发), 因此内部无需任何同步。
 */
class NodeTimeWindowCache {
 public:
  /**
   * @brief pick/drop 缓存条目
   * @details arena_slot 是本条目 orders 所在的 arena 槽位, 淘汰时回收复用;
   * tick 是最近一次访问的序号, 批次 LRU 按它挑最旧的一批淘汰。
   */
  struct PickDropEntry {
    std::vector<TimeWindow*> tws;
    int arena_slot{0};
    long long tick{0};
  };

  using PickDropMap = std::unordered_map<NodeTimeWindowKey, PickDropEntry>;

  const PlanDatetimeRange* plan_datetime_range;
  /**
   * @brief 缓存容量
   * @details <= 0 表示不限制容量, 此时退化为无淘汰的纯缓存。
   */
  const int cache_capacity;
  /**
   * @brief 入库 key 的 orders 存储
   * @details 内层 vector 的堆缓冲在 arena 扩容时保持有效, 故 key 可安全持有其 data();
   * 声明在缓存表之前, 析构时晚于缓存表释放。槽位只 clear 不 erase, 否则其余 key 的
   * orders 视图会失效。
   */
  std::vector<std::vector<const Order*>> orders_arena;

  /**
   * @brief 淘汰后回收的 arena 槽位, 优先于新建槽位被复用
   */
  std::vector<int> orders_arena_free_slots;

  PickDropMap pick_drop_cache;

  std::unordered_map<NodeTimeWindowKey, std::vector<TimeWindow*>> other_cache;

  /**
   * @brief 构造查找 key 时复用的订单缓冲, 容量稳定后每次查找零分配
   */
  std::vector<const Order*> orders_scratch;

  /**
   * @brief 单调递增的访问序号, 命中/入库时写入条目 tick
   */
  long long access_clock{0};

  explicit NodeTimeWindowCache(const PlanDatetimeRange* plan_datetime_range, const int cache_capacity)
      : plan_datetime_range(plan_datetime_range), cache_capacity(cache_capacity), pick_drop_cache(), other_cache() {
    this->pick_drop_cache.reserve(CacheParameter::LRU_RESERVED_ENTRY_COUNT);
  }

  ~NodeTimeWindowCache();

  /**
   * @brief 取一个可用的 orders 槽位, 优先复用被淘汰的槽位
   */
  int acquire_orders_slot();

  /**
   * @brief 条目数超容量时淘汰最旧的一批
   */
  void evict_pick_drop_overflow();

  /**
   * @brief 淘汰单个条目: 释放时间窗 + 回收 arena 槽位
   */
  PickDropMap::iterator erase_pick_drop(PickDropMap::iterator it);

  /**
   * @brief 取节点的订单交集时间窗
   * @details 返回的 TimeWindow* 归缓存所有, 条目被淘汰时随之释放; 调用方只能在本次
   * 推导中使用, 不得跨缓存操作长期持有。
   */
  std::vector<TimeWindow*> get_pick_drop_time_windows(const Node* node);

  std::vector<TimeWindow*> get_other_time_windows(const Node* node);
};
