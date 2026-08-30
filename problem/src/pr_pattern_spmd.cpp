/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#include <utility>

#include "pr_pattern.h"

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

Load* PatternSPMD::create_load(LoadContext* context) const { return new LoadSPMD(context); }

bool PatternSPMD::insert_last_drop(Load* load, const Order* order) const {
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
      Node* cur_drop_node_ptr = cur_drop_node.get();
      // link C->prev = A
      cur_drop_node->prev = last_drop_node;
      // link B->prev = C
      load->last_node->prev = cur_drop_node_ptr;
      // link C->next = B
      cur_drop_node->next = std::move(last_drop_node->next);
      // link A->next = C
      last_drop_node->next = std::move(cur_drop_node);
    } else {
      last_drop_node->add_front_activity(std::move(activities_it.second));
    }
  }
  return true;
}
