/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <memory>
#include <vector>

#include "dm_order.h"
#include "dm_predefine.h"

/**
 * @brief activity is action for order in node
 */
class Activity {
 public:
  using UPtr = std::unique_ptr<Activity>;
  using VecUPtr = std::vector<UPtr>;
  /**
   * @brief activity type
   */
  const ActivityType activity_type;
  /**
   * @brief order of activity
   */
  const Order* order;
  /**
   * @brief previous activity
   */
  Activity* prev;
  /**
   * @brief next activity
   */
  Activity::UPtr next;
  /**
   * @brief related activity
   */
  Activity* related;
  /**
   * @brief related node
   */
  Node* related_node;

  Activity();

  Activity(const ActivityType activity_type, const Order* order);

  void set_prev(Activity* prev);

  void set_next(Activity::UPtr next);

  void set_related(Activity* related);

  void set_related_node(Node* related_node);

  bool hase_next();

  bool hase_prev();
};

namespace ActivityFactory {
std::pair<Activity::UPtr, Activity::UPtr> create_pair_activity(const Order* order);
}
