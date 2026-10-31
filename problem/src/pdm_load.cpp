/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#include "pdm_load.h"

#include <ranges>

#include "bdm_time_window_plan.h"
#include "bdm_time_window_utils.h"

// ====== implement of LoadContext ======
LoadContext::LoadContext(const Scenario* scenario, const Parameter* parameter)
    : scenario(scenario),
      plan_datetime_range(parameter->plan_datetime_range),
      cache(new Cache(parameter)) {}

LoadContext::~LoadContext() {
  delete this->cache;
  this->cache = nullptr;
  this->scenario = nullptr;
}

// ====== implement of Load ======
Load::Load(LoadContext* context) : context(context), dist_matrix_code(nullptr), vehicle(nullptr) {
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
  this->dist_matrix_code = other->dist_matrix_code;
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

void Load::reset_route_profile() const { this->route_profile->reset_dirty_marks(); }

void Load::change_vehicle(const Vehicle* _vehicle) {
  // check if the routing network has changed
  // if change, update the distance and time between nodes
  bool network_change_flag = false;
  if (this->vehicle != nullptr && this->dist_matrix_code != this->vehicle->get_dist_matrix_code()) {
    network_change_flag = true;
  }
  this->vehicle = _vehicle;
  this->dist_matrix_code = this->vehicle->get_dist_matrix_code();

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
    this->route_profile->reset_dirty_mark(LoadRouteProfileField::TOTAL_TRAVEL_DIST);
  } else if (change_start_node_flag) {
    this->route_profile->reset_dirty_mark(LoadRouteProfileField::TIME_WINDOW);
    this->update_start_node_dist_time();
  } else if (change_end_node_flag) {
    this->route_profile->reset_dirty_mark(LoadRouteProfileField::TIME_WINDOW);
    this->update_end_node_dist_time();
  }
}

long Load::get_total_travel_dist() const {
  if (this->route_profile->get_set_dirty_mark(LoadRouteProfileField::TOTAL_TRAVEL_DIST)) {
    return this->route_profile->total_travel_dist;
  }
  long total_travel_dist = 0;
  Node* tail_node = this->last_node;
  while (tail_node->hase_prev()) {
    total_travel_dist += tail_node->travel_dist;
    tail_node = tail_node->prev;
  }
  this->route_profile->total_travel_dist = total_travel_dist;
  return total_travel_dist;
}

long Load::get_total_travel_time() const {
  if (this->route_profile->get_set_dirty_mark(LoadRouteProfileField::TOTAL_TRAVEL_TIME)) {
    return this->route_profile->total_travel_time;
  }
  long total_travel_time = 0;
  Node* tail_node = this->last_node;
  while (tail_node->hase_prev()) {
    total_travel_time += tail_node->travel_time;
    tail_node = tail_node->prev;
  }
  this->route_profile->total_travel_time = total_travel_time;
  return total_travel_time;
}

double Load::get_obj_val() const { return this->constr_profile->obj_val; }

Bitset* Load::get_available_vehicle_bitset() const {
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

bool Load::is_infeasible() const { return this->constr_profile->is_infeasible(); }

bool Load::is_feasible() const { return this->constr_profile->is_feasible(); }

bool Load::is_empty() const { return this->first_node->next.get() == this->last_node; }

NodeTimeWindowStoreBuffer* Load::get_node_tw_store_buffer() const {
  if (this->store_buffer == nullptr) {
    return nullptr;
  }
  return this->store_buffer->node_time_window;
}

void Load::update_node_dist_time() const {
  if (this->route_profile->get_dirty_mark(LoadRouteProfileField::NODE_DIST_TIME)) {
    return;
  }

  const DistMatrix* dist_matrix = this->vehicle->get_dist_matrix();
  Node* tail_node = this->last_node;
  while (tail_node->hase_prev()) {
    Node* prev_node = tail_node->prev;
    auto [tmp_dist, tmp_time] =
        dist_matrix->get_dist_time(prev_node->loc->ind, tail_node->loc->ind);

    tail_node->travel_dist = tmp_dist;
    tail_node->travel_time = tmp_time;
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

void Load::update_end_node_dist_time() const {
  const DistMatrix* dist_matrix = this->vehicle->get_dist_matrix();
  auto dist_time =
      dist_matrix->get_dist_time(this->last_node->prev->loc->ind, this->last_node->loc->ind);
  this->last_node->travel_dist = dist_time.dist;
  this->last_node->travel_time = dist_time.time;

  // todo updata time window
}

void Load::update_time_window() const {
  auto* tw_cache = this->context->cache->node_time_window;
  // 第一个node(车场，可能为虚拟车场)
  Node* pre_node = this->first_node.get();
  pre_node->ptws = TimeWindowPlanFactory::create_time_window_plans(
      tw_cache->lookup(this->get_node_tw_store_buffer(), pre_node));
  Node* head_node = pre_node->next.get();
  while (head_node != nullptr) {
    // 冻结表 →写缓冲→ 重算并写入写缓冲; 返回的引用在本次推导内有效
    const std::vector<TimeWindow*>& cur_tws =
        tw_cache->lookup(this->get_node_tw_store_buffer(), head_node);
    const long work_time = head_node->get_work_time();
    head_node->ptws =
        TimeWindowInfer::forward(pre_node->ptws, head_node->travel_time, work_time, cur_tws);
    pre_node = head_node;
    head_node = head_node->next.get();
  }
}

void Load::remove_activities(const std::vector<Activity*>& activities) const {
  // 借 Activity::owner_node 直接定位所属 node, 无需遍历整条链
  for (Activity* activity : activities) {
    Node* cur_node = activity->owner_node;
    if (cur_node->remove_activity(activity)) {
      // node 已无 activity: 摘除并释放(cur_node 必为前驱 next 所指)
      NodeOps::splice_out(cur_node->prev->next, cur_node);
    }
  }
}

std::vector<Node*> Load::unfold_node_linked() const {
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

void Load::fill_locations(std::vector<const Location*>& locations) const {
  const auto* node = this->first_node->next.get();
  const auto* pre_node = this->first_node.get();
  while (node != this->last_node) {
    if (!(pre_node->loc == node->loc && pre_node->activity_type == node->activity_type)) {
      locations.push_back(node->loc);
    }
    pre_node = node;
    node = node->next.get();
  }
}
