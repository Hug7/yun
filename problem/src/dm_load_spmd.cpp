/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#include "dm_load.h"

// ====== implement of Load SPMD ======
LoadSPMD::~LoadSPMD() {}

int LoadSPMD::get_pick_node_count() {
  return this->get_pick_node_count_sp();
}

int LoadSPMD::get_drop_node_count() {
  return this->get_drop_node_count_md();
}

const std::vector<long>& LoadSPMD::get_peak_load_dims() {
  return this->get_peak_load_dims_sp();
}

LabelsetValueBitset* LoadSPMD::get_pick_loc_labelset_value_bitset() {
  return this->get_pick_loc_labelset_value_bitset_sp();
}

LabelsetValueBitset* LoadSPMD::get_drop_loc_labelset_value_bitset() {
  return this->get_drop_loc_labelset_value_bitset_md();
}

LabelsetValueBitset* LoadSPMD::get_order_labelset_value_bitset() {
  return this->get_order_labelset_value_bitset_sp();
}
