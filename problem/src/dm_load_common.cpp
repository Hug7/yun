/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#include "dm_load.h"

// ====== implement of Load ======
const std::vector<long>& Load::get_peak_load_dims_sp() {
  std::vector<long>& peak_load_dims = this->route_profile->peak_load_dims;
  if (this->route_profile->get_set_dirty_mark(LoadRouteProfileField::PEAK_LOAD_DIMS)) {
    return peak_load_dims;
  }
  const int dim_size = peak_load_dims.size();
  const Node* head_node = this->first_node->next.get();
  if (head_node->activity_type == ActivityType::PICK) {
    auto tail_activity = head_node->last;
    while (tail_activity != nullptr) {
      auto& cur_load_dims = tail_activity->order->dim_vals;
      for (int u = 0; u < dim_size; u++) {
        peak_load_dims[u] += cur_load_dims[u];
      }
      tail_activity = tail_activity->prev;
    }
  }

  return peak_load_dims;
}

const std::vector<long>& Load::get_peak_load_dims_sd() {
  std::vector<long>& peak_load_dims = this->route_profile->peak_load_dims;
  if (this->route_profile->get_set_dirty_mark(LoadRouteProfileField::PEAK_LOAD_DIMS)) {
    return peak_load_dims;
  }
  const int dim_size = peak_load_dims.size();
  const Node* tail_node = this->last_node->prev;
  if (tail_node->activity_type == ActivityType::DROP) {
    auto tail_activity = tail_node->last;
    while (tail_activity != nullptr) {
      auto& cur_load_dims = tail_activity->order->dim_vals;
      for (int u = 0; u < dim_size; u++) {
        peak_load_dims[u] += cur_load_dims[u];
      }
      tail_activity = tail_activity->prev;
    }
  }

  return peak_load_dims;
}

const std::vector<long>& Load::get_peak_load_dims_mp() {
  std::vector<long>& peak_load_dims = this->route_profile->peak_load_dims;
  if (this->route_profile->get_set_dirty_mark(LoadRouteProfileField::PEAK_LOAD_DIMS)) {
    return peak_load_dims;
  }
  const int dim_size = peak_load_dims.size();
  auto head_node = this->first_node->next.get();
  while (head_node->activity_type == ActivityType::PICK) {
    auto tail_activity = head_node->last;
    while (tail_activity != nullptr) {
      auto& cur_load_dims = tail_activity->order->dim_vals;
      for (int u = 0; u < dim_size; u++) {
        peak_load_dims[u] += cur_load_dims[u];
      }
      tail_activity = tail_activity->prev;
    }
    head_node = head_node->next.get();
  }

  return peak_load_dims;
}

int Load::get_pick_node_count_sp() {
  if (this->route_profile->get_set_dirty_mark(LoadRouteProfileField::PICK_NODE_COUNT)) {
    return this->route_profile->pick_node_count;
  }
  if (this->first_node->next->activity_type == ActivityType::PICK) {
    this->route_profile->pick_node_count = 1;
    return 1;
  }
  this->route_profile->pick_node_count = 0;
  return 0;
}

int Load::get_pick_node_count_mp() {
  if (this->route_profile->get_set_dirty_mark(LoadRouteProfileField::PICK_NODE_COUNT)) {
    return this->route_profile->pick_node_count;
  }
  auto head_node = this->first_node->next.get();
  int tmp_pick_node_count = 0;
  while (head_node->activity_type == ActivityType::PICK) {
    head_node = head_node->next.get();
    ++tmp_pick_node_count;
  }
  this->route_profile->pick_node_count = tmp_pick_node_count;
  return tmp_pick_node_count;
}

int Load::get_drop_node_count_sd() {
  if (this->route_profile->get_set_dirty_mark(LoadRouteProfileField::DROP_NODE_COUNT)) {
    return this->route_profile->drop_node_count;
  }
  if (this->last_node->prev->activity_type == ActivityType::DROP) {
    this->route_profile->drop_node_count = 1;
    return 1;
  }
  this->route_profile->drop_node_count = 0;
  return 0;
}

int Load::get_drop_node_count_md() {
  if (this->route_profile->get_set_dirty_mark(LoadRouteProfileField::DROP_NODE_COUNT)) {
    return this->route_profile->drop_node_count;
  }
  int tmp_drop_node_count = 0;
  Node* head_node = this->last_node->prev;
  while (head_node->activity_type == ActivityType::DROP) {
    ++tmp_drop_node_count;
    head_node = head_node->prev;
  }
  this->route_profile->drop_node_count = tmp_drop_node_count;
  return tmp_drop_node_count;
}

LabelsetValueBitset* Load::get_pick_loc_labelset_value_bitset_sp() {
  if (this->route_profile->get_set_dirty_mark(
          LoadRouteProfileField::PICK_LOC_LABELSET_VALUE_BITSET)) {
    return this->route_profile->pick_loc_labelset_value_bitset.get();
  }
  this->route_profile->pick_loc_labelset_value_bitset->clear_bitsets();
  Node* head_node = this->first_node->next.get();
  if (head_node->activity_type == ActivityType::PICK) {
    this->route_profile->pick_loc_labelset_value_bitset->merge(
        head_node->loc->labelset_value_bitset);
  }
  return this->route_profile->pick_loc_labelset_value_bitset.get();
}

LabelsetValueBitset* Load::get_pick_loc_labelset_value_bitset_mp() {
  if (this->route_profile->get_set_dirty_mark(
          LoadRouteProfileField::PICK_LOC_LABELSET_VALUE_BITSET)) {
    return this->route_profile->pick_loc_labelset_value_bitset.get();
  }
  this->route_profile->pick_loc_labelset_value_bitset->clear_bitsets();
  Node* head_node = this->first_node->next.get();
  while (head_node->activity_type == ActivityType::PICK) {
    this->route_profile->pick_loc_labelset_value_bitset->merge(
        head_node->loc->labelset_value_bitset);
    head_node = head_node->next.get();
  }
  return this->route_profile->pick_loc_labelset_value_bitset.get();
}

LabelsetValueBitset* Load::get_drop_loc_labelset_value_bitset_sd() {
  if (this->route_profile->get_set_dirty_mark(
          LoadRouteProfileField::DROP_LOC_LABELSET_VALUE_BITSET)) {
    return this->route_profile->drop_loc_labelset_value_bitset.get();
  }
  this->route_profile->drop_loc_labelset_value_bitset->clear_bitsets();
  Node* tail_node = this->last_node->prev;
  if (tail_node->activity_type == ActivityType::DROP) {
    this->route_profile->drop_loc_labelset_value_bitset->merge(
        tail_node->loc->labelset_value_bitset);
  }
  return this->route_profile->drop_loc_labelset_value_bitset.get();
}

LabelsetValueBitset* Load::get_drop_loc_labelset_value_bitset_md() {
  if (this->route_profile->get_set_dirty_mark(
          LoadRouteProfileField::DROP_LOC_LABELSET_VALUE_BITSET)) {
    return this->route_profile->drop_loc_labelset_value_bitset.get();
  }
  this->route_profile->drop_loc_labelset_value_bitset->clear_bitsets();
  Node* tail_node = this->last_node->prev;
  while (tail_node->activity_type == ActivityType::DROP) {
    this->route_profile->drop_loc_labelset_value_bitset->merge(
        tail_node->loc->labelset_value_bitset);
    tail_node = tail_node->prev;
  }
  return this->route_profile->drop_loc_labelset_value_bitset.get();
}

LabelsetValueBitset* Load::get_order_labelset_value_bitset_sp() {
  if (this->route_profile->get_set_dirty_mark(LoadRouteProfileField::ORDER_LABELSET_VALUE_BITSET)) {
    return this->route_profile->order_labelset_value_bitset.get();
  }
  this->route_profile->order_labelset_value_bitset->clear_bitsets();
  Node* head_node = this->first_node->next.get();
  if (head_node->activity_type == ActivityType::PICK) {
    auto tail_activity = head_node->last;
    while (tail_activity != nullptr) {
      this->route_profile->pick_loc_labelset_value_bitset->merge(
          tail_activity->order->labelset_value_bitset);
      tail_activity = tail_activity->prev;
    }
  }
  return this->route_profile->order_labelset_value_bitset.get();
}

LabelsetValueBitset* Load::get_order_labelset_value_bitset_sd() {
  if (this->route_profile->get_set_dirty_mark(LoadRouteProfileField::ORDER_LABELSET_VALUE_BITSET)) {
    return this->route_profile->order_labelset_value_bitset.get();
  }
  this->route_profile->order_labelset_value_bitset->clear_bitsets();
  Node* tail_node = this->last_node->prev;
  if (tail_node->activity_type == ActivityType::DROP) {
    auto tail_activity = tail_node->last;
    while (tail_activity != nullptr) {
      this->route_profile->pick_loc_labelset_value_bitset->merge(
          tail_activity->order->labelset_value_bitset);
      tail_activity = tail_activity->prev;
    }
    tail_node = tail_node->prev;
  }
  return this->route_profile->order_labelset_value_bitset.get();
}

LabelsetValueBitset* Load::get_order_labelset_value_bitset_mp() {
  if (this->route_profile->get_set_dirty_mark(LoadRouteProfileField::ORDER_LABELSET_VALUE_BITSET)) {
    return this->route_profile->order_labelset_value_bitset.get();
  }
  this->route_profile->order_labelset_value_bitset->clear_bitsets();
  Node* head_node = this->first_node->next.get();
  while (head_node->activity_type == ActivityType::PICK) {
    auto tail_activity = head_node->last;
    while (tail_activity != nullptr) {
      this->route_profile->pick_loc_labelset_value_bitset->merge(
          tail_activity->order->labelset_value_bitset);
      tail_activity = tail_activity->prev;
    }
    head_node = head_node->next.get();
  }
  return this->route_profile->order_labelset_value_bitset.get();
}
