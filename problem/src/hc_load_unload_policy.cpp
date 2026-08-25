/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#include <unordered_map>

#include "hc_load_unload_policy.h"

// ====== implement of HcLoadUnloadPolicyFILO ======
HardConstrScore::UPtr HcLoadUnloadPolicyFILO::eval(Load* load) {
  if (load->get_pick_node_count() == 1 || load->get_drop_node_count() == 1) {
    return std::make_unique<HardConstrScore>(code);
  }
  Node* head_node = load->first_node->next.get();
  std::unordered_map<int, int> order_indices;
  int node_ind = -1;
  int top_pick_node_ind = 0;
  while (head_node->hase_next()) {
    ++node_ind;
    if (head_node->activity_type == ActivityType::PICK) {
      auto tail_activity = head_node->last;
      if (tail_activity != nullptr) {
        top_pick_node_ind = node_ind;
      }
      while (tail_activity != nullptr) {
        order_indices.emplace(tail_activity->order->ind, node_ind);
        tail_activity = tail_activity->prev;
      }
    }
    if (head_node->activity_type == ActivityType::DROP) {
      auto tail_activity = head_node->last;
      int tmp_top_pick_node_ind = top_pick_node_ind;
      while (tail_activity != nullptr) {
        const int cur_order_ind = tail_activity->order->ind;
        auto it = order_indices.find(cur_order_ind);
        if (it == order_indices.end()) {
          return std::make_unique<HardConstrScore>(
              this->code, false, 1, 1, this->seq_type, this->vehicle_type,
              InfeasibleReasonCollection::FILO_LOAD_UNLOAD_POLICY);
        }
        if (it->second > top_pick_node_ind) {
          return std::make_unique<HardConstrScore>(
              this->code, false, 1, 1, this->seq_type, this->vehicle_type,
              InfeasibleReasonCollection::FILO_LOAD_UNLOAD_POLICY);
        }
        tail_activity = tail_activity->prev;
        tmp_top_pick_node_ind = std::min(tmp_top_pick_node_ind, it->second);
      }
      top_pick_node_ind = tmp_top_pick_node_ind;
    }

    head_node = head_node->next.get();
  }

  return std::make_unique<HardConstrScore>(code);
}

// ====== implement of HcLoadUnloadPolicyFIFO ======
HardConstrScore::UPtr HcLoadUnloadPolicyFIFO::eval(Load* load) {
  if (load->get_pick_node_count() == 1 || load->get_drop_node_count() == 1) {
    return std::make_unique<HardConstrScore>(code);
  }
  Node* head_node = load->first_node->next.get();
  std::unordered_map<int, int> order_indices;
  int node_ind = -1;
  int top_pick_node_ind = 0;
  while (head_node->hase_next()) {
    ++node_ind;
    if (head_node->activity_type == ActivityType::PICK) {
      auto tail_activity = head_node->last;
      while (tail_activity != nullptr) {
        order_indices.emplace(tail_activity->order->ind, node_ind);
        tail_activity = tail_activity->prev;
      }
    }
    if (head_node->activity_type == ActivityType::DROP) {
      auto tail_activity = head_node->last;
      int tmp_top_pick_node_ind = top_pick_node_ind;
      while (tail_activity != nullptr) {
        const int cur_order_ind = tail_activity->order->ind;
        auto it = order_indices.find(cur_order_ind);
        if (it == order_indices.end()) {
          return std::make_unique<HardConstrScore>(
              this->code, false, 1, 1, this->seq_type, this->vehicle_type,
              InfeasibleReasonCollection::FIFO_LOAD_UNLOAD_POLICY);
        }
        if (it->second < top_pick_node_ind) {
          return std::make_unique<HardConstrScore>(
              this->code, false, 1, 1, this->seq_type, this->vehicle_type,
              InfeasibleReasonCollection::FIFO_LOAD_UNLOAD_POLICY);
        }
        tail_activity = tail_activity->prev;
        tmp_top_pick_node_ind = std::max(tmp_top_pick_node_ind, it->second);
      }
      top_pick_node_ind = tmp_top_pick_node_ind;
    }

    head_node = head_node->next.get();
  }

  return std::make_unique<HardConstrScore>(code);
}
