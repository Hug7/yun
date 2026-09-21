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
Load::Load(LoadContext* context)
    : context(context), prev_dist_matrix_code(nullptr), vehicle(nullptr) {
  Location* default_loc = context->scenario->location_manager->get_default_location();
  this->first_node = std::make_unique<Node>(ActivityType::START, default_loc);
  auto end_node = std::make_unique<Node>(ActivityType::END, default_loc);
  end_node->prev = this->first_node.get();
  this->first_node->next = std::move(end_node);
  this->last_node = this->first_node->next.get();
  this->route_profile = std::make_unique<LoadRouteProfile>(context->scenario);
  this->constr_profile = std::make_unique<LoadConstrProfile>();
}

Load::Load(const Load* other) {
  // copy-context
  this->context = other->context;
  // copy-距离矩阵编码
  this->prev_dist_matrix_code = other->prev_dist_matrix_code;
  // copy-车辆
  this->vehicle = other->vehicle;
  // copy-route属性
  this->route_profile = std::make_unique<LoadRouteProfile>(other->route_profile);
  // copy-约束+成本属性
  this->constr_profile = std::make_unique<LoadConstrProfile>(other->constr_profile);
  // copy-node链(含首尾depot哨兵), 并修正尾节点指针
  this->first_node = NodeOps::deep_copy_chain(other->first_node.get());
  this->last_node = NodeOps::tail(this->first_node);
}

Load::~Load() {
  this->first_node.reset();
  this->last_node = nullptr;
  this->route_profile.reset();
  this->constr_profile.reset();
}

void Load::reset_route_profile() const {
  this->route_profile->reset_dirty_marks();
}

void Load::change_vehicle(const Vehicle* _vehicle) {
  // check if the routing network has changed
  // if change, update the distance and time between nodes
  bool network_change_flag = false;
  if (this->vehicle != nullptr &&
      this->prev_dist_matrix_code != this->vehicle->get_dist_matrix_code()) {
    network_change_flag = true;
  }
  this->vehicle = _vehicle;
  this->prev_dist_matrix_code = this->vehicle->get_dist_matrix_code();

  // change start node
  const bool change_start_node_flag = this->first_node->loc != this->vehicle->orig_loc;
  if (change_start_node_flag) {
    auto node = std::make_unique<Node>(ActivityType::START, this->vehicle->orig_loc);
    this->first_node->next->prev = node.get();
    node->next = std::move(this->first_node->next);
    this->first_node = std::move(node);
  }
  // change end node
  const bool change_end_node_flag = this->last_node->loc != this->vehicle->dest_loc;
  if (change_end_node_flag) {
    auto node = std::make_unique<Node>(ActivityType::END, this->vehicle->dest_loc);
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

long Load::get_total_dist() const {
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

bool Load::is_infeasible() const {
  return this->constr_profile->is_infeasible();
}

bool Load::is_feasible() const {
  return this->constr_profile->is_feasible();
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
    pre_node = head_node;
    head_node = head_node->next.get();
  }
}

std::vector<Node*> Load::unfold_node_linked() const{
  std::vector<Node*> reverse_nodes;
  auto tail_node = this->last_node;
  while (tail_node != nullptr) {
    reverse_nodes.push_back(tail_node);
    tail_node = tail_node->prev;
  }
  std::ranges::reverse(reverse_nodes);
  return reverse_nodes;
}

TimeWindowPlan::VecUPtr Load::infer_time_window_plans() const {
  const std::vector<Node*>& nodes = this->unfold_node_linked();
  // 推导时间窗-反向推导(非最优且不能兼容限制时间窗)
  const int node_len = static_cast<int>(nodes.size());
  TimeWindowPlan::VecUPtr time_window_plans;
  time_window_plans.push_back(nodes[node_len - 1]->ptws[0]->deep_copy());
  int pre_ptw_ind = 0;
  auto pre_node = nodes[node_len - 1];
  for (int nu = node_len - 2; nu >= 0; --nu) {
    const auto cur_node = nodes[nu];
    auto cur_time_window_plan = std::make_unique<TimeWindowPlan>();
    cur_time_window_plan->early_depart =
        time_window_plans[pre_ptw_ind]->early_arr - pre_node->travel_time;
    cur_time_window_plan->late_depart =
        time_window_plans[pre_ptw_ind]->late_arr - pre_node->travel_time;
    long cur_early_arr = cur_time_window_plan->early_depart - cur_node->get_work_time();
    long cur_late_arr = cur_time_window_plan->late_depart - cur_node->get_work_time();
    for (const auto& twp : cur_node->ptws) {
      if (twp->wait_time > 0) {
        cur_time_window_plan->early_arr = cur_early_arr - twp->wait_time;
        cur_time_window_plan->late_arr = cur_late_arr - twp->wait_time;
        cur_time_window_plan->wait_time = twp->wait_time;
        break;
      } else if (twp->over_time > 0) {
        cur_time_window_plan->early_arr = cur_early_arr;
        cur_time_window_plan->late_arr = cur_late_arr;
        cur_time_window_plan->over_time = twp->over_time;
        break;
      } else if (twp->early_arr <= cur_early_arr && twp->late_arr >= cur_late_arr) {
        cur_time_window_plan->early_arr = cur_early_arr;
        cur_time_window_plan->late_arr = cur_late_arr;
        break;
      }
    }
    pre_node = cur_node;
    ++pre_ptw_ind;
    time_window_plans.push_back(std::move(cur_time_window_plan));
  }
  std::ranges::reverse(time_window_plans);

  return time_window_plans;
}
