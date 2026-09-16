/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#include "pdm_load.h"

// ====== implement of Load SPMD ======
LoadSPMD::~LoadSPMD() = default;

int LoadSPMD::get_pick_node_count() { return this->get_pick_node_count_sp(); }

int LoadSPMD::get_drop_node_count() { return this->get_drop_node_count_md(); }

const std::vector<long>& LoadSPMD::get_peak_load_dims() { return this->get_peak_load_dims_sp(); }

LabelsetValueBitset* LoadSPMD::get_pick_loc_labelset_value_bitset() {
  return this->get_pick_loc_labelset_value_bitset_sp();
}

LabelsetValueBitset* LoadSPMD::get_drop_loc_labelset_value_bitset() {
  return this->get_drop_loc_labelset_value_bitset_md();
}

LabelsetValueBitset* LoadSPMD::get_order_labelset_value_bitset() {
  return this->get_order_labelset_value_bitset_sp();
}

std::vector<const Order*> LoadSPMD::get_all_orders() {
  if (this->first_node->next->activity_type != ActivityType::PICK) {
    return {};
  }
  std::vector<const Order*> res;
  auto tail_activity = this->first_node->next->last;
  while (tail_activity != nullptr) {
    res.push_back(tail_activity->order);
    tail_activity = tail_activity->prev;
  }
  return res;
}
