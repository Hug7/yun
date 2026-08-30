/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#include "pr_pattern.h"

#include "pdm_node.h"

Node* PickDropPattern::find_first_pick_node(std::unique_ptr<Node>& node) const {
  Node* head_node = node.get();
  while (head_node != nullptr) {
    if (head_node->is_pick()) {
      return head_node;
    }
    head_node = head_node->next.get();
  }
  return nullptr;
}

Node* PickDropPattern::find_last_pick_node(std::unique_ptr<Node>& node) const {
  Node* head_node = node.get();
  Node* res_node = nullptr;
  while (head_node != nullptr) {
    if (head_node->is_pick()) {
      res_node = head_node;
    }
    head_node = head_node->next.get();
  }
  return res_node;
}

Node* PickDropPattern::find_first_drop_node(std::unique_ptr<Node>& node) const {
  Node* head_node = node.get();
  while (head_node != nullptr) {
    if (head_node->is_drop()) {
      return head_node;
    }
    head_node = head_node->next.get();
  }
  return nullptr;
}

Node* PickDropPattern::find_last_drop_node(std::unique_ptr<Node>& node) const {
  Node* head_node = node.get();
  Node* res_node = nullptr;
  while (head_node != nullptr) {
    if (head_node->is_drop()) {
      res_node = head_node;
    }
    head_node = head_node->next.get();
  }
  return res_node;
}

Node* PickDropPattern::find_load_first_pick_node(Load* load) const {
  Node* node = load->first_node->next.get();
  while (node != nullptr) {
    if (node->is_pick()) {
      return node;
    }
    node = node->next.get();
  }
  return nullptr;
}

Node* PickDropPattern::find_load_last_pick_node(Load* load) const {
  Node* node = load->last_node->prev;
  while (node != nullptr) {
    if (node->is_pick()) {
      return node;
    }
    node = node->prev;
  }
  return nullptr;
}

Node* PickDropPattern::find_load_first_drop_node(Load* load) const {
  Node* node = load->first_node->next.get();
  while (node != nullptr) {
    if (node->is_drop()) {
      return node;
    }
    node = node->next.get();
  }
  return nullptr;
}

Node* PickDropPattern::find_load_last_drop_node(Load* load) const {
  Node* node = load->last_node->prev;
  while (node != nullptr) {
    if (node->is_drop()) {
      return node;
    }
    node = node->prev;
  }
  return nullptr;
}
