/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#include "dm_load.h"

// ====== implement of Load ======
Load::Load(const Scenario* scenario) : scenario(scenario) {
  Location* default_loc = scenario->location_manager->get_default_location();
  this->first_node = std::make_unique<Node>(ActivityType::START, default_loc);
  auto end_node = std::make_unique<Node>(ActivityType::END, default_loc);
  end_node->prev = this->first_node.get();
  this->first_node->next = std::move(end_node);
  this->last_node = end_node.get();
  this->route_profile = std::make_unique<LoadRouteProfile>(scenario);
  this->constr_profile = std::make_unique<LoadConstrProfile>();
}

void Load::change_vehicle(Vehicle* vehicle) {
  // check if the routing network has changed
  // if change, update the distance and time between nodes
  bool network_change_flag = false;
  if (this->vehicle != nullptr ||
      this->prev_dist_matrix_code != this->vehicle->get_dist_matrix_code()) {
    this->prev_dist_matrix_code = this->vehicle->get_dist_matrix_code();
    network_change_flag = true;
  }
  this->vehicle = vehicle;

  // change start node
  const bool change_start_node_flag = this->first_node->loc != vehicle->orig_loc;
  if (change_start_node_flag) {
    auto node = std::make_unique<Node>(ActivityType::START, vehicle->orig_loc);
    this->first_node->next->prev = node.get();
    node->next = std::move(this->first_node->next);
    this->first_node = std::move(node);
  }
  // change end node
  const bool change_end_node_flag = this->last_node->loc != vehicle->dest_loc;
  if (change_end_node_flag) {
    auto node = std::make_unique<Node>(ActivityType::END, vehicle->dest_loc);
    Node* node_ptr = node.get();
    node->prev = this->last_node->prev;
    this->last_node->prev->next = std::move(node);
    this->last_node = node_ptr;
  }

  if (network_change_flag) {
    this->update_node_dist_time();
  } else if (change_start_node_flag) {
    this->update_start_node_dist_time();
  } else if (change_end_node_flag) {
    this->update_end_node_dist_time();
  }
}

long Load::get_total_dist() {
  if (this->route_profile->get_set_dirty_mark(LoadRouteProfileField::TOTAL_DIST)) {
    return this->route_profile->total_dist;
  }
  long total_dist = 0;
  Node* tail_node = this->last_node;
  while (tail_node->hase_prev()) {
    total_dist += tail_node->travel_dist;
    tail_node = tail_node->prev;
  }
  this->route_profile->total_dist = total_dist;
  return total_dist;
}

void Load::update_node_dist_time() {
  const DistMatrix* dist_matrix = this->vehicle->get_dist_matrix();
  Node* tail_node = this->last_node;
  while (tail_node->hase_prev()) {
    Node* prev_node = tail_node->prev;
    auto dist_time = dist_matrix->get_dist_time(prev_node->loc->ind, tail_node->loc->ind);

    tail_node->travel_dist = dist_time.dist;
    tail_node->travel_time = dist_time.time;
    tail_node = prev_node;
  }

  // todo updata time window
}

void Load::update_start_node_dist_time() {
  const DistMatrix* dist_matrix = this->vehicle->get_dist_matrix();
  auto dist_time =
      dist_matrix->get_dist_time(this->first_node->loc->ind, this->first_node->next->loc->ind);
  this->first_node->next->travel_dist = dist_time.dist;
  this->first_node->next->travel_time = dist_time.time;

  // todo updata time window
}

void Load::update_end_node_dist_time() {
  const DistMatrix* dist_matrix = this->vehicle->get_dist_matrix();
  auto dist_time =
      dist_matrix->get_dist_time(this->last_node->prev->loc->ind, this->last_node->loc->ind);
  this->last_node->travel_dist = dist_time.dist;
  this->last_node->travel_time = dist_time.time;

  // todo updata time window
}
