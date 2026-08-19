/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#include "pr_pattern.h"

Node* PickDropPattern::find_first_pick_node(std::unique_ptr<Node>& node) {
  Node* head_node = node.get();
  while (head_node != nullptr) {
    if (head_node->activity_type == ActivityType::PICK) {
      return head_node;
    }
    head_node = head_node->next.get();
  }
  return nullptr;
}

Node* PickDropPattern::find_last_pick_node(std::unique_ptr<Node>& node) {
  Node* head_node = node.get();
  Node* res_node = nullptr;
  while (head_node != nullptr) {
    if (head_node->activity_type == ActivityType::PICK) {
      res_node = head_node;
    }
    head_node = head_node->next.get();
  }
  return res_node;
}

Node* PickDropPattern::find_first_drop_node(std::unique_ptr<Node>& node) {
  Node* head_node = node.get();
  while (head_node != nullptr) {
    if (head_node->activity_type == ActivityType::DROP) {
      return head_node;
    }
    head_node = head_node->next.get();
  }
  return nullptr;
}

Node* PickDropPattern::find_last_drop_node(std::unique_ptr<Node>& node) {
  Node* head_node = node.get();
  Node* res_node = nullptr;
  while (head_node != nullptr) {
    if (head_node->activity_type == ActivityType::DROP) {
      res_node = head_node;
    }
    head_node = head_node->next.get();
  }
  return res_node;
}
