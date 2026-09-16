/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#include "pdm_node.h"

#include <unordered_map>

#include "bdm_time_window.h"
#include "bdm_time_window_utils.h"

// ====== implement of Node ======

Node::Node(const ActivityType activity_type, const Location* loc)
    : activity_type(activity_type), loc(loc) {
  this->ptws = std::vector<std::unique_ptr<TimeWindowPlan>>();
  this->first = nullptr;
  this->last = nullptr;
  this->next = nullptr;
  this->prev = nullptr;
  this->travel_dist = 0;
  this->travel_time = 0;
}

Node::Node(const ActivityType activity_type, const Location* loc,
           std::unique_ptr<Activity> activity)
    : activity_type(activity_type), loc(loc) {
  this->ptws = std::vector<std::unique_ptr<TimeWindowPlan>>();
  this->first = std::move(activity);
  this->last = this->first.get();
  this->next = nullptr;
  this->prev = nullptr;
  this->travel_dist = 0;
  this->travel_time = 0;
}

void Node::set_travel_dist(long travel_dist) { this->travel_dist = travel_dist; }

void Node::set_travel_time(long travel_time) { this->travel_time = travel_time; }

void Node::add_front_activity(Activity::UPtr activity) {
  if (this->first != nullptr) {
    this->first->prev = activity.get();
    activity->next = std::move(this->first);
    this->first = std::move(activity);
  } else {
    this->first = std::move(activity);
    this->last = this->first.get();
  }
}

void Node::add_back_activity(Activity::UPtr activity) {
  if (this->last != nullptr) {
    activity->prev = this->last;
    this->last->next = std::move(activity);
    this->last = this->last->next.get();
  } else {
    this->first = std::move(activity);
    this->last = this->first.get();
  }
}

std::vector<TimeWindow*> Node::intersection_time_windows() const {
  if (this->first == nullptr) {
    return {};
  }
  std::vector<std::vector<TimeWindow*>> ori_tws_arr;
  if (this->activity_type == ActivityType::PICK) {
    auto tail_activity = this->last;
    while (tail_activity) {
      ori_tws_arr.push_back(tail_activity->order->pick_time_windows);
      tail_activity = tail_activity->prev;
    }
  } else if (this->activity_type == ActivityType::DROP) {
    auto tail_activity = this->last;
    while (tail_activity) {
      ori_tws_arr.push_back(tail_activity->order->drop_time_windows);
      tail_activity = tail_activity->prev;
    }
  }
  return TimeWindowUntils::intersection_tws_arr(ori_tws_arr);
}

std::vector<const Order*> Node::get_orders() const {
  std::vector<const Order*> orders;
  auto tail_activity = this->last;
  while (tail_activity) {
    orders.push_back(tail_activity->order);
    tail_activity = tail_activity->prev;
  }
  return orders;
}

long Node::get_work_time() const {
  long work_time = 0;
  if (this->activity_type == ActivityType::DROP) {
    work_time += this->loc->work_plan->fixed_drop_time;
    auto tail_activity = this->last;
    while (tail_activity) {
      work_time += tail_activity->order->drop_work_time;
      tail_activity = tail_activity->prev;
    }
  } else if (this->activity_type == ActivityType::PICK) {
    work_time += this->loc->work_plan->fixed_pick_time;
    auto tail_activity = this->last;
    while (tail_activity) {
      work_time += tail_activity->order->pick_work_time;
      tail_activity = tail_activity->prev;
    }
  }
  return work_time;
}

// ====== implement of NodeFactory ======
std::unique_ptr<Node> NodeFactory::create_pick_node(const Order* order, Activity::UPtr activity) {
  return std::make_unique<Node>(ActivityType::PICK, order->pick_loc, std::move(activity));
}

std::unique_ptr<Node> NodeFactory::create_drop_node(const Order* order, Activity::UPtr activity) {
  return std::make_unique<Node>(ActivityType::DROP, order->drop_loc, std::move(activity));
}

std::pair<Node::UPtr, Node::UPtr> NodeFactory::create_pair_node(const Order* order) {
  auto activities_it = ActivityFactory::create_pair_activity(order);
  auto pick_node = std::make_unique<Node>(ActivityType::PICK, order->pick_loc);
  auto drop_node = std::make_unique<Node>(ActivityType::DROP, order->drop_loc);
  pick_node->add_front_activity(std::move(activities_it.first));
  drop_node->add_back_activity(std::move(activities_it.second));

  return std::make_pair(std::move(pick_node), std::move(drop_node));
}

// ====== implement of NodeFactory ======
Node* NodeOps::tail(const Node::UPtr& head) {
  if (!head) {
    return nullptr;
  }

  Node* cur = head.get();
  while (cur->next) {
    cur = cur->next.get();
  }

  return cur;
}

Node::UPtr NodeOps::splice_out(Node::UPtr& from_ptr, Node* to) {
  auto subchain = std::move(from_ptr);  // 摘下整段
  Node* pred = subchain->prev;          // 记住前驱

  from_ptr = std::move(to->next);  // 后半段接回前驱

  if (from_ptr) {
    from_ptr->prev = pred;  // 后半段头节点的 prev
  }

  subchain->prev = nullptr;  // 独立链无前驱
  return subchain;
}

void NodeOps::splice_in_after(Node* after, Node::UPtr chain) {
  if (!chain) {
    return;
  }

  Node* t = tail(chain);

  t->next = std::move(after->next);  // chain 尾部接原后继
  if (t->next) {
    t->next->prev = t;
  }

  chain->prev = after;             // chain 头部指回 after
  after->next = std::move(chain);  // after 接 chain
}

Node::UPtr NodeOps::reverse_chain(Node::UPtr chain) {
  if (!chain || !chain->next) {
    return chain;
  }
  Node::UPtr prev_p;
  Node::UPtr curr = std::move(chain);
  while (curr) {
    auto nxt = std::move(curr->next);
    curr->next = std::move(prev_p);
    if (curr->next) {
      curr->next->prev = curr.get();  // 翻转后 prev 也跟着翻
    }
    prev_p = std::move(curr);
    curr = std::move(nxt);
  }

  if (prev_p) {
    prev_p->prev = nullptr;
  }
  return prev_p;
}

Node::UPtr NodeOps::deep_copy_chain(const Node* head) {
  if (head == nullptr) {
    return nullptr;
  }
  std::unordered_map<const Node*, Node*> node_map;
  std::unordered_map<const Activity*, Activity*> activity_map;

  // 第一遍: 逐个克隆 node 及其 activity 子链, 并记录新旧指针的映射
  Node::UPtr copy_head;
  Node* copy_tail = nullptr;
  for (const Node* src_node = head; src_node != nullptr; src_node = src_node->next.get()) {
    auto copy_node = std::make_unique<Node>(src_node->activity_type, src_node->loc);
    for (const auto& src_ptw : src_node->ptws) {
      copy_node->ptws.push_back(src_ptw->deep_copy());
    }
    for (const Activity* src_activity = src_node->first.get(); src_activity != nullptr;
         src_activity = src_activity->next.get()) {
      auto copy_activity =
          std::make_unique<Activity>(src_activity->activity_type, src_activity->order);
      activity_map[src_activity] = copy_activity.get();
      copy_node->add_back_activity(std::move(copy_activity));
    }
    copy_node->travel_dist = src_node->travel_dist;
    copy_node->travel_time = src_node->travel_time;
    copy_node->prev = copy_tail;

    Node* copy_node_ptr = copy_node.get();
    if (copy_tail == nullptr) {
      copy_head = std::move(copy_node);
    } else {
      copy_tail->next = std::move(copy_node);
    }
    copy_tail = copy_node_ptr;
    node_map[src_node] = copy_tail;
  }

  // 第二遍: 修正跨 node 的关联指针(取货 activity <-> 送货 activity)
  for (const Node* src_node = head; src_node != nullptr; src_node = src_node->next.get()) {
    for (const Activity* src_activity = src_node->first.get(); src_activity != nullptr;
         src_activity = src_activity->next.get()) {
      Activity* copy_activity = activity_map.at(src_activity);
      if (src_activity->related != nullptr) {
        const auto related_it = activity_map.find(src_activity->related);
        copy_activity->related = related_it == activity_map.end() ? nullptr : related_it->second;
      }
      if (src_activity->related_node != nullptr) {
        const auto node_it = node_map.find(src_activity->related_node);
        copy_activity->related_node = node_it == node_map.end() ? nullptr : node_it->second;
      }
    }
  }

  return copy_head;
}
