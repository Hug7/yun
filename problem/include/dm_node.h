/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <memory>
#include <vector>

#include "bd_location.h"
#include "bd_time_window_plan.h"
#include "c_constant.h"
#include "dm_activity.h"
#include "dm_order.h"
#include "dm_predefine.h"

/**
 * @brief node
 */
class Node {
 public:
  using UPtr = std::unique_ptr<Node>;
  using VecUPtr = std::vector<UPtr>;
  /**
   * @brief activity type
   */
  const ActivityType activity_type;
  /**
   * @brief location of the node
   */
  const Location* loc;
  /**
   * @brief plan time windows of the node
   */
  TimeWindowPlan::VecUPtr ptws;
  /**
   * @brief first activity in the node
   */
  Activity::UPtr first;
  /**
   * @brief last activity in the node
   */
  Activity* last;
  /**
   * @brief previous node
   */
  Node* prev;
  /**
   * @brief next node
   */
  Node::UPtr next;
  /**
   * @brief distance for previous node to this node
   */
  long travel_dist;
  /**
   * @brief time for previous node to this node
   */
  long travel_time;

  Node();

  Node(const ActivityType activity_type, const Location* loc);

  Node(const ActivityType activity_type, const Location* loc, Activity::UPtr activity);

  void set_travel_dist(long travel_dist);

  void set_travel_time(long travel_time);

  bool hase_next();

  bool hase_prev();

  void add_front_activity(Activity::UPtr activity);

  void add_back_activity(Activity::UPtr activity);
};

namespace NodeFactory {
Node::UPtr create_pick_node(const Order* order, Activity::UPtr activity);

Node::UPtr create_drop_node(const Order* order, Activity::UPtr activity);

std::pair<Node::UPtr, Node::UPtr> create_pair_node(const Order* order);
}  // namespace NodeFactory

struct NodeOps {
  static Node* tail(Node::UPtr& head);

  // 从链表中摘除子链 [from_ptr 指向的节点 ... to]
  // from_ptr: 即 Load::first_node 或 前驱节点->next
  // 执行后 from_ptr 指向 to 后面的节点
  // 返回: 被摘下的独立子链 (prev 已置 nullptr)
  static Node::UPtr splice_out(Node::UPtr& from_ptr, Node* to);

  // 将 chain 插入 after 后面
  // 自动修正 chain 内部所有 prev、以及 after->next 的 prev
  static void splice_in_after(Node* after, Node::UPtr chain);

  // 翻转独立链。返回新头 (原尾)。修正内部所有 prev
  static Node::UPtr reverse_chain(Node::UPtr chain);
};
