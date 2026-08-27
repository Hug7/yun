/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <unordered_map>
#include <vector>

#include "bd_location.h"
#include "bd_time_window.h"
#include "c_constant.h"
#include "dm_node.h"
#include "dm_order.h"
#include "dm_parameter.h"

/**
 * @brief 节点时间窗键
 */
class NodeTimeWindowKey {
 public:
  const ActivityType activity_type;

  const Location* location;

  std::vector<const Order*> orders;

  int order_len;

  int hash_code;

  NodeTimeWindowKey(const ActivityType activity_type, const Location* location);

  NodeTimeWindowKey(const ActivityType activity_type, const Location* location,
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
    return this->hash_code == other.hash_code;
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
 * @details 时间窗缓存
 */
class NodeTimeWindowCache {
 public:
  const PlanDatetimeRange* plan_datetime_range;

  std::unordered_map<NodeTimeWindowKey*, std::vector<TimeWindow*>> pick_drop_cache;

  std::unordered_map<NodeTimeWindowKey*, std::vector<TimeWindow*>> other_cache;

  NodeTimeWindowCache(const PlanDatetimeRange* plan_datetime_range)
      : plan_datetime_range(plan_datetime_range), pick_drop_cache(), other_cache() {}

  ~NodeTimeWindowCache();

  std::vector<TimeWindow*> get_pick_drop_time_windows(Node* node);

  std::vector<TimeWindow*> get_other_time_windows(Node* node);
};
