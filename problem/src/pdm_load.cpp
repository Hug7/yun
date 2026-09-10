/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#include "pdm_load.h"

#include "bdm_time_window_plan.h"
#include "bdm_time_window_utils.h"

// ====== implement of LoadContext ======
LoadContext::LoadContext(const Scenario* scenario, const PlanDatetimeRange* plan_datetime_range)
    : scenario(scenario) {
  this->node_time_window_cache = new NodeTimeWindowCache(plan_datetime_range);
}

LoadContext::~LoadContext() {
  this->scenario = nullptr;
  delete this->node_time_window_cache;
}

// ====== implement of Load ======
Load::Load(LoadContext* context) : context(context) {
  Location* default_loc = context->scenario->location_manager->get_default_location();
  this->first_node = std::make_unique<Node>(ActivityType::START, default_loc);
  auto end_node = std::make_unique<Node>(ActivityType::END, default_loc);
  end_node->prev = this->first_node.get();
  this->last_node = end_node.get();
  this->first_node->next = std::move(end_node);
  this->route_profile = std::make_unique<LoadRouteProfile>(context->scenario);
  this->constr_profile = std::make_unique<LoadConstrProfile>();
}

Load::~Load() {
  this->first_node.reset();
  this->last_node = nullptr;
  this->route_profile.reset();
  this->constr_profile.reset();
}

void Load::reset_route_profile() {
  this->route_profile->reset_dirty_marks();
}

void Load::change_vehicle(Vehicle* vehicle) {
  // check if the routing network has changed
  // if change, update the distance and time between nodes
  bool network_change_flag = false;
  if (this->vehicle != nullptr &&
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
    this->route_profile->reset_dirty_mark(LoadRouteProfileField::TIME_WINDOW);
    this->route_profile->reset_dirty_mark(LoadRouteProfileField::NODE_DIST_TIME);
    this->route_profile->reset_dirty_mark(LoadRouteProfileField::TOTAL_DIST);
  } else if (change_start_node_flag) {
    this->route_profile->reset_dirty_mark(LoadRouteProfileField::TIME_WINDOW);
    this->update_start_node_dist_time();
  } else if (change_end_node_flag) {
    this->route_profile->reset_dirty_mark(LoadRouteProfileField::TIME_WINDOW);
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

Bitset* Load::get_available_vehicle_bitset() {
  if (this->route_profile->get_set_dirty_mark(LoadRouteProfileField::AVAILABLE_VEHICLE_BITSET)) {
    return this->route_profile->available_vehicle_bitset.get();
  }
  this->route_profile->available_vehicle_bitset =
      this->context->scenario->carrier_manager->full_vehicle_bitset();
  Node* tail_node = this->last_node->prev;
  while (tail_node->hase_prev()) {
    this->route_profile->available_vehicle_bitset->call_intersection(
        tail_node->loc->available_vehicle->vehicle_bitset);
    tail_node = tail_node->prev;
  }

  return this->route_profile->available_vehicle_bitset.get();
}

void Load::update_node_dist_time() {
  if (this->route_profile->get_dirty_mark(LoadRouteProfileField::NODE_DIST_TIME)) {
    return;
  }

  const DistMatrix* dist_matrix = this->vehicle->get_dist_matrix();
  Node* tail_node = this->last_node;
  while (tail_node->hase_prev()) {
    Node* prev_node = tail_node->prev;
    auto dist_time = dist_matrix->get_dist_time(prev_node->loc->ind, tail_node->loc->ind);

    tail_node->travel_dist = dist_time.dist;
    tail_node->travel_time = dist_time.time;
    tail_node = prev_node;
  }
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

void Load::update_time_window() {
  const auto tw_cache = this->context->node_time_window_cache;
  // 第一个node(车场，可能为虚拟车场)
  Node* pre_node = this->first_node.get();
  pre_node->ptws = TimeWindowPlanFactory::create_time_window_plans(tw_cache->get_other_time_windows(pre_node));
  Node* head_node = pre_node->next.get();
  while (head_node != nullptr) {
    auto cur_tws = tw_cache->get_pick_drop_time_windows(head_node);
    const long work_time = head_node->get_work_time();
    head_node->ptws = TimeWindowInfer::forward(pre_node->ptws, head_node->travel_time, work_time, cur_tws);
    head_node = head_node->next.get();
  }
}
