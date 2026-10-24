/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#include "pdm_load.h"

// ====== implement of Load SPMD ======
LoadSPMD::~LoadSPMD() = default;

int LoadSPMD::get_pick_node_count() { return this->get_pick_node_count_sp(); }

int LoadSPMD::get_drop_node_count() { return this->get_drop_node_count_md(); }

const std::vector<long>& LoadSPMD::get_peak_load_dims() const { return this->get_peak_load_dims_sp(); }

const std::vector<long>& LoadSPMD::get_total_load_dims() const { return this->get_peak_load_dims_sp(); }

LabelsetValueBitset* LoadSPMD::get_pick_loc_labelset_value_bitset() {
  return this->get_pick_loc_labelset_value_bitset_sp();
}

LabelsetValueBitset* LoadSPMD::get_drop_loc_labelset_value_bitset() {
  return this->get_drop_loc_labelset_value_bitset_md();
}

LabelsetValueBitset* LoadSPMD::get_order_labelset_value_bitset() {
  return this->get_order_labelset_value_bitset_sp();
}

std::vector<const Order*> LoadSPMD::get_orders() {
  std::vector<const Order*> res;
  this->fill_orders(res);
  return res;
}

void LoadSPMD::fill_orders(std::vector<const Order*>& orders) {
  if (this->first_node->next->activity_type != ActivityType::PICK) {
    return;
  }
  auto tail_activity = this->first_node->next->last;
  while (tail_activity != nullptr) {
    orders.push_back(tail_activity->order);
    tail_activity = tail_activity->prev;
  }
}

std::vector<Activity*> LoadSPMD::get_pick_activities() {
  if (this->first_node->next->activity_type != ActivityType::PICK) {
    return {};
  }
  std::vector<Activity*> pick_activities;
  auto* tail_activity = this->first_node->next->last;
  while (tail_activity != nullptr) {
    pick_activities.push_back(tail_activity);
    tail_activity = tail_activity->prev;
  }
  return pick_activities;
}

int LoadSPMD::get_order_count() const {
  if (this->first_node->next->activity_type != ActivityType::PICK) {
    return 0;
  }
  return this->first_node->next->get_activity_count();
}

int LoadSPMD::get_activity_count() const {
  if (this->first_node->next->activity_type != ActivityType::PICK) {
    return 0;
  }
  return this->first_node->next->get_activity_count() * 2;
}
