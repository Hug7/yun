/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#include "dm_time_window_cache.h"

// ====== implement of NodeTimeWindowKey ======
NodeTimeWindowKey::NodeTimeWindowKey(const ActivityType activity_type, const Location* location,
                                     const std::vector<const Order*>& orders)
    : activity_type(activity_type), location(location), orders(orders) {
  this->order_len = orders.size();
  this->hash_code = static_cast<int>(activity_type) + 1;
  this->hash_code = this->hash_code * 31 + location->ind;
  for (int u = 0; u < order_len; u++) {
    this->hash_code = this->hash_code * 31 + orders[u]->ind;
  }
}

// ====== implement of NodeTimeWindowCache ======
NodeTimeWindowCache::~NodeTimeWindowCache() {
  for (auto& it : this->cache) {
    for (auto& tws : it.second) {
      delete tws;
    }
  }
  this->cache.clear();
}

std::vector<TimeWindow*> NodeTimeWindowCache::get_time_windows(Node* node) {
  auto node_time_window_key =
      new NodeTimeWindowKey(node->activity_type, node->loc, node->get_orders());
  auto it = this->cache.find(node_time_window_key);
  if (it != this->cache.end()) {
    delete node_time_window_key;
    return it->second;
  } else {
    
  }
}
