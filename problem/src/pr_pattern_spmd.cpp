/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#include "pr_pattern.h"

Load* SPMD::create_load(const Scenario* scenario) { return new LoadSPMD(scenario); }

bool SPMD::add_order(Load* load, Order* order) {
  Node* first_pick_node = find_first_pick_node(load->first_node);
  if (first_pick_node == nullptr) {
    // case of empty load
    auto node_pair_it = NodeFactory::create_pair_node(order);
    load->last_node = node_pair_it.second.get();
    load->first_node->next = std::move(node_pair_it.first);
    load->first_node->next->next = std::move(node_pair_it.second);
  } else {
    if (first_pick_node->loc != order->pick_loc) {
      // case: SPMD not allow exist multi pick location
      return false;
    }
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
