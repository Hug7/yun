/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#include "hc_location.h"

#include <unordered_set>
#include <utility>
#include <vector>

#include "bdm_available_vehicle.h"
#include "pdm_load.h"
#include "pdm_segment.h"

namespace {

/**
 * @brief 收集车次链上提货节点与卸货节点的站点索引
 */
std::pair<std::unordered_set<int>, std::unordered_set<int>> collect_node_loc_inds(Load* load) {
  std::unordered_set<int> pick_locs;
  std::unordered_set<int> drop_locs;
  for (Node* node = load->first_node.get(); node != nullptr; node = node->next.get()) {
    if (node->loc == nullptr) {
      continue;
    }
    if (node->is_pick()) {
      pick_locs.insert(node->loc->ind);
    } else if (node->is_drop()) {
      drop_locs.insert(node->loc->ind);
    }
  }
  return {std::move(pick_locs), std::move(drop_locs)};
}

/**
 * @brief segment 内连续同站点订单构成的站点 run
 * @details 连续的相同站点会在插入时合并成一个节点, 因此按 run 而不是按订单计数
 */
std::vector<int> segment_loc_runs(const Segment* segment, const bool pick) {
  std::vector<int> runs;
  for (const Order* order : segment->orders) {
    const Location* loc = pick ? order->pick_loc : order->drop_loc;
    if (runs.empty() || runs.back() != loc->ind) {
      runs.push_back(loc->ind);
    }
  }
  return runs;
}

/**
 * @brief 段内是否存在缺失站点
 */
bool has_missing_loc(const Segment* segment, const bool pick) {
  for (const Order* order : segment->orders) {
    if ((pick ? order->pick_loc : order->drop_loc) == nullptr) {
      return true;
    }
  }
  return false;
}

/**
 * @brief 节点数约束的预检
 * @details 上界: 每个站点 run 至多新增一个节点;
 * 下界: 站点不在车次链上的 run 无法与既有节点合并, 必定各新增一个节点。
 */
HcPrecheckVerdict precheck_node_count(Load* load, const Segment* segment, const bool pick,
                                      const int base_count, const int max_count) {
  if (segment->order_count == 0) {
    return HcPrecheckVerdict::kPass;
  }
  if (has_missing_loc(segment, pick)) {
    return HcPrecheckVerdict::kUnknown;
  }
  const std::vector<int> runs = segment_loc_runs(segment, pick);
  const int run_len = static_cast<int>(runs.size());
  if (base_count + run_len <= max_count) {
    return HcPrecheckVerdict::kPass;
  }

  const auto [pick_locs, drop_locs] = collect_node_loc_inds(load);
  const std::unordered_set<int>& existing = pick ? pick_locs : drop_locs;
  int new_nodes = 0;
  for (const int loc_ind : runs) {
    if (!existing.contains(loc_ind)) {
      ++new_nodes;
    }
  }
  if (base_count + new_nodes > max_count) {
    return HcPrecheckVerdict::kInfeasible;
  }
  return HcPrecheckVerdict::kUnknown;
}

}  // namespace

// ====== implement of HcMaxPickNodeCount ======
HardConstrScore::UPtr HcMaxPickNodeCount::eval(Load* load) {
  if (load->get_pick_node_count() > this->max_pick_node_count) {
    return std::make_unique<HardConstrScore>(this->code, false, 1, 1, this->seq_type,
                                             this->vehicle_type,
                                             InfeasibleReasonCollection::MAX_PICK_NODE_COUNT);
  }
  return std::make_unique<HardConstrScore>(this->code);
}

HcPrecheckVerdict HcMaxPickNodeCount::precheck(const HcPrecheckInput& input) const {
  return precheck_node_count(input.load, input.segment, true, input.load->get_pick_node_count(),
                             this->max_pick_node_count);
}

// ====== implement of HcMaxDropNodeCount ======
HardConstrScore::UPtr HcMaxDropNodeCount::eval(Load* load) {
  if (load->get_drop_node_count() > this->max_drop_node_count) {
    return std::make_unique<HardConstrScore>(this->code, false, 1, 1, this->seq_type,
                                             this->vehicle_type,
                                             InfeasibleReasonCollection::MAX_DROP_NODE_COUNT);
  }
  return std::make_unique<HardConstrScore>(this->code);
}

HcPrecheckVerdict HcMaxDropNodeCount::precheck(const HcPrecheckInput& input) const {
  return precheck_node_count(input.load, input.segment, false, input.load->get_drop_node_count(),
                             this->max_drop_node_count);
}

// ====== implement of HcAvailableVehicle ======
HardConstrScore::UPtr HcAvailableVehicle::eval(Load* load) {
  const auto& available_vehicle_bitset = load->get_available_vehicle_bitset();

  if (!available_vehicle_bitset.test(load->vehicle->ind)) {
    return std::make_unique<HardConstrScore>(this->code, false, 1, 1, this->seq_type,
                                             this->vehicle_type,
                                             InfeasibleReasonCollection::AVAILABLE_VEHICLE);
  }

  return std::make_unique<HardConstrScore>(this->code);
}

HcPrecheckVerdict HcAvailableVehicle::precheck(const HcPrecheckInput& input) const {
  // 在基线可用车型位图上继续与 segment 涉及站点的可用车型求交
  Bitset available = input.load->get_available_vehicle_bitset();
  for (const Order* order : input.segment->orders) {
    for (const Location* loc : {order->pick_loc, order->drop_loc}) {
      if (loc->available_vehicle == nullptr) {
        continue;
      }
      available.call_intersection(loc->available_vehicle->vehicle_bitset);
    }
    if (available.sum() == 0) {
      return HcPrecheckVerdict::kInfeasible;
    }
  }
  return HcPrecheckVerdict::kPass;
}

// ====== implement of HcTimeWindow ======
HardConstrScore::UPtr HcTimeWindow::eval(Load* load) {
  // 更新站点间距离和时间
  load->update_node_dist_time();
  // 更新并时间窗
  load->update_time_window();

  auto tail_node = load->last_node;
  while (tail_node != nullptr) {
    if (tail_node->ptws.empty() || tail_node->ptws[0].over_time > 0) {
      return std::make_unique<HardConstrScore>(this->code, false, 1, 1, this->seq_type,
                                               this->vehicle_type,
                                               InfeasibleReasonCollection::TIME_WINDOW);
    }
    tail_node = tail_node->prev;
  }

  return std::make_unique<HardConstrScore>(this->code);
}
