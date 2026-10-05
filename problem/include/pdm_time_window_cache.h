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
  const PlanDatetimeRange* plan_datetime_range;

  /**
   * @brief 入库 key 的 orders 存储
   * @details 内层 vector 的堆缓冲在 arena 扩容时保持有效, 故 key 可安全持有其 data();
   * 声明在缓存表之前, 析构时晚于缓存表释放。
   */
  std::vector<std::vector<const Order*>> orders_arena;

  std::unordered_map<NodeTimeWindowKey, std::vector<TimeWindow*>> pick_drop_cache;

  std::unordered_map<NodeTimeWindowKey, std::vector<TimeWindow*>> other_cache;

  /**
   * @brief 构造查找 key 时复用的订单缓冲, 容量稳定后每次查找零分配
   */
  std::vector<const Order*> orders_scratch;

  explicit NodeTimeWindowCache(const PlanDatetimeRange* plan_datetime_range)
      : plan_datetime_range(plan_datetime_range), pick_drop_cache(), other_cache() {}

  ~NodeTimeWindowCache();

  std::vector<TimeWindow*> get_pick_drop_time_windows(const Node* node);

  std::vector<TimeWindow*> get_other_time_windows(const Node* node);
};
