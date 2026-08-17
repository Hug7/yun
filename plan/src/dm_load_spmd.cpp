/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#include "dm_load.h"

// ====== implement of Load SPMD ======
int LoadSPMD::get_pick_node_count() {
  if (this->route_profile->get_set_dirty_mark(LoadRouteProfileField::PICK_NODE_COUNT)) {
    return this->route_profile->pick_node_count;
  }
  Node* head_node = this->first_node->next.get();
  if (head_node->activity_type == ActivityType::PICK) {
    this->route_profile->pick_node_count = 1;
    return 1;
  }
  this->route_profile->pick_node_count = 0;
  return 0;
}

int LoadSPMD::get_drop_node_count() {
  if (this->route_profile->get_set_dirty_mark(LoadRouteProfileField::DROP_NODE_COUNT)) {
    return this->route_profile->pick_node_count;
  }
  int drop_node_count = 0;
  Node* head_node = this->last_node->prev;
  while (head_node->activity_type == ActivityType::DROP) {
    ++drop_node_count;
    head_node = head_node->prev;
  }
  this->route_profile->drop_node_count = drop_node_count;
  return drop_node_count;
}

const std::vector<long>& LoadSPMD::get_peak_load_dims() {
  std::vector<long>& peak_load_dims = this->route_profile->peak_load_dims;
  if (this->route_profile->get_set_dirty_mark(LoadRouteProfileField::PEAK_LOAD_DIMS)) {
    return peak_load_dims;
  }
  int dim_size = peak_load_dims.size();
  Node* head_node = this->first_node->next.get();
  if (head_node->activity_type == ActivityType::PICK) {
    Activity* head_activity = head_node->first.get();
    const std::vector<long>& cur_load_dims = head_activity->order->dim_vals;
    for (int u = 0; u < dim_size; u++) {
      peak_load_dims[u] += cur_load_dims[u];
    }
    head_activity = head_activity->next.get();
  }

  return peak_load_dims;
}
