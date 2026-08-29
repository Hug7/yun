/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#include "pr_policy.h"
#include <utility>

bool PolicyFILO::add_order(Load* load, const Order* order) {
  Node* first_pick_node = this->pattern->find_first_pick_node(load->first_node);
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
    auto activities_it = ActivityFactory::create_pair_activity(order);
    
    first_pick_node->add_front_activity(std::move(activities_it.first));
    if (load->last_node->loc != order->drop_loc) {
      auto drop_node = NodeFactory::create_drop_node(order, std::move(activities_it.second));
      Node* drop_node_ptr = drop_node.get();
      drop_node->prev = load->last_node;
      load->last_node->next = std::move(drop_node);
      load->last_node = drop_node_ptr;
    }
  }

  return false;
}

bool PolicyFIFO::add_order(Load* load, const Order* order) {

}

bool PolicyNone::add_order(Load* load, const Order* order) {

}
