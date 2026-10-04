/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#include <utility>
#include <vector>

#include "prob_pattern.h"

// ====== implement of PatternSPMD ======
Node* PatternSPMD::find_load_first_pick_node(Load* load) const {
  auto& node = load->first_node->next;
  if (node->is_pick()) {
    return node.get();
  }
  return nullptr;
}

Node* PatternSPMD::find_load_last_pick_node(Load* load) const {
  auto& node = load->first_node->next;
  if (node->is_pick()) {
    return node.get();
  }
  return nullptr;
}

Node* PatternSPMD::find_load_first_drop_node(Load* load) const {
  auto& node = load->first_node->next;
  if (node->is_pick()) {
    return node->next.get();
  }
  return nullptr;
}

Node* PatternSPMD::find_load_last_drop_node(Load* load) const {
  auto node = load->last_node->prev;
  if (node->is_drop()) {
    return node;
  }
  return nullptr;
}

std::vector<InsertSlot> PatternSPMD::enumerate_insert_slots(Load* load,
                                                            const Segment* segment) const {
  const std::vector<const Order*>& orders = segment->orders;
  if (orders.empty()) {
    return {};
  }
  // SPMD 下 segment 内所有订单必须共享同一提货点, 且与 load 唯一的 pick node 一致
  const Location* pick_loc = orders.front()->pick_loc;
  for (const Order* order : orders) {
    if (order->pick_loc != pick_loc) {
      return {};
    }
  }
  const Node* first_pick_node = this->find_load_first_pick_node(load);
  if (first_pick_node != nullptr && first_pick_node->loc != pick_loc) {
    return {};
  }
  // 送货侧是唯一的自由度: k 个 drop node 对应 k+1 个插入位
  const int drop_node_count = load->get_drop_node_count();
  std::vector<InsertSlot> slots;
  slots.reserve(drop_node_count + 1);
  for (int ind = 0; ind <= drop_node_count; ++ind) {
    slots.emplace_back(ActivityType::DROP, ind);
  }
  return slots;
}

std::vector<Activity*> PatternSPMD::insert_segment_at(Load* load, const Segment* segment,
                                                      const InsertSlot& slot) const {
  if (load->is_empty()) {
    throw std::runtime_error(
        "PickDropPattern function insert_segment_at not allow `load` is empty!");
  }
  if (segment->order_count == 0) {
    throw std::runtime_error(
        "PickDropPattern function insert_segment_at not allow `segment` is empty!");
  }
  const std::vector<const Order*>& orders = segment->orders;
  // 提货侧: SPMD 下全部并入唯一 pick node
  Node* pick_node = this->find_load_first_pick_node(load);
  if (pick_node->loc != orders.front()->pick_loc) {
    return {};
  }
  // 解析锚点: 新片段插在 anchor 之后。首尾 depot 哨兵会被 change_vehicle 替换, 故每次重新解析
  Node* anchor = load->last_node->prev;
  for (int ind = 0; ind < slot.ind; ++ind) {
    anchor = anchor->prev;
  }
  Node* cursor = anchor;
  Node* successor = anchor->next.get();

  std::vector<Activity*> inserted;
  inserted.reserve(segment->order_count * 2);

  for (int u = 0; u < segment->order_count; ++u) {
    const Order* order = orders[u];
    auto [pick_activity, drop_activity] = ActivityFactory::create_pair_activity(order);
    inserted.push_back(pick_activity.get());
    inserted.push_back(drop_activity.get());
    pick_node->add_back_activity(std::move(pick_activity));

    if (cursor->is_drop() && cursor->loc == order->drop_loc) {
      cursor->add_back_activity(std::move(drop_activity));
    } else if (successor->is_drop() && successor->loc == order->drop_loc) {
      successor->add_back_activity(std::move(drop_activity));
      cursor = successor;
      successor = successor->next.get();
    } else {
      auto drop_node = NodeFactory::create_drop_node(order, std::move(drop_activity));
      Node* new_drop_node = drop_node.get();
      NodeOps::splice_in_after(cursor, std::move(drop_node));
      cursor = new_drop_node;
    }
  }

  return inserted;
}

LoadSPMD* PatternSPMD::create_load(LoadContext* context) const { return new LoadSPMD(context); }

LoadSPMD* PatternSPMD::deep_copy_load(const Load* other) const { return new LoadSPMD(other); }

bool PatternSPMD::insert_last_delivery(Load* load, const Order* order) const {
  // 重置route profile
  load->reset_route_profile();
  // 找到第一个pick node
  Node* first_pick_node = this->find_load_first_pick_node(load);
  if (first_pick_node == nullptr) {
    // case of empty load
    auto node_pair_it = NodeFactory::create_pair_node(order);
    // connect previous
    node_pair_it.first->prev = load->first_node.get();
    node_pair_it.second->prev = node_pair_it.first.get();
    load->last_node->prev = node_pair_it.second.get();
    // connect next
    node_pair_it.second->next = std::move(load->first_node->next);
    node_pair_it.first->next = std::move(node_pair_it.second);
    load->first_node->next = std::move(node_pair_it.first);
  } else {
    // 校验第一个pick node和order的pick loc是否一致
    if (first_pick_node->loc != order->pick_loc) {
      return false;
    }
    auto activities_it = ActivityFactory::create_pair_activity(order);
    // A->B, insert C to A->C->B
    // A=last drop node, B=last node, C=curent drop node
    first_pick_node->add_front_activity(std::move(activities_it.first));
    auto last_drop_node = load->last_node->prev;
    if (last_drop_node->loc != order->drop_loc) {
      auto cur_drop_node = NodeFactory::create_drop_node(order, std::move(activities_it.second));
      // Node* cur_drop_node_ptr = cur_drop_node.get();
      // link C->prev = A
      cur_drop_node->prev = last_drop_node;
      // link C->next = B
      cur_drop_node->next = std::move(last_drop_node->next);
      // link A->next = C
      last_drop_node->next = std::move(cur_drop_node);
      // link B->prev = C
      load->last_node->prev = last_drop_node->next.get();
    } else {
      last_drop_node->add_front_activity(std::move(activities_it.second));
    }
  }
  return true;
}
