/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <memory>
#include <vector>

#include "pdm_order.h"
#include "pdm_predefine.h"

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
  UPtr next;
  /**
   * @brief related activity
   */
  Activity* related;
  /**
   * @brief 所属 node
   * @details 由 Node 的构造函数/add_front_activity/add_back_activity 维护, 摘除 activity 后该指针随
   * activity 一起销毁; 深拷贝时由 add_*_activity 重映射到克隆链
   */
  Node* owner_node;

  Activity();

  Activity(ActivityType activity_type, const Order* order);

  void set_prev(Activity* prev);

  void set_next(Activity::UPtr next);

  void set_related(Activity* related);

  void set_owner_node(Node* owner_node);

  bool hase_next();

  bool hase_prev();
};

namespace ActivityFactory {
std::pair<Activity::UPtr, Activity::UPtr> create_pair_activity(const Order* order);
}
