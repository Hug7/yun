/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#include "prob_load_editor.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <memory>
#include <stdexcept>
#include <vector>

#include "c_rang_utils.h"
#include "prob_pattern.h"
#include "prob_problem.h"

namespace {

/**
 * @brief 试插可行解数量达到槽位数的比例后提前停止搜索
 * @details 槽位已按 delta dist 升序排列, 先试连接处距离变化最小的位置;
 * 可行解凑够该比例即认为样本足够
 */
constexpr double INSERT_FEASIBLE_STOP_RATIO = 0.2;

}  // namespace

// ====== implement of LoadEditor ======
Load* LoadEditor::build(const std::vector<const Order*>& orders,
                        const VehicleResource& vehicle_resource) const {
  Load* load = this->problem->pd_pattern->create_load(this->problem->load_context);
  for (const auto order : orders) {
    this->problem->pd_pattern->insert_last_delivery(load, order);
  }
  // try using different vehicle
  LoadConstrProfile::UPtr best_profile = nullptr;
  const Vehicle* best_vehicle = nullptr;
  for (const auto& vehicle_usage : vehicle_resource.vehicle_usages) {
    // check vehicle resource
    if (!vehicle_usage->has_available_vehicle()) {
      continue;
    }
    // change vehicle
    load->change_vehicle(vehicle_usage->vehicle);
    // temporary evaluate of load
    auto cur_constr_profile = this->problem->evaluate(load);
    if (cur_constr_profile->is_infeasible()) {
      continue;
    }
    if (best_vehicle == nullptr || cur_constr_profile->dominate(best_profile)) {
      best_profile = std::move(cur_constr_profile);
      best_vehicle = vehicle_usage->vehicle;
    }
  }
  // 没有任何可行车辆时不能切换车辆，直接标记不可行交由调用方处理
  if (best_vehicle == nullptr) {
    load->constr_profile->set_infeasible();
    return load;
  }
  // change vehicle
  load->change_vehicle(best_vehicle);
  this->problem->eval_load(load);

  return load;
}

Load* LoadEditor::build(const std::vector<const Order*>& orders, const Vehicle* vehicle,
                        CacheStoreBuffer* store_buffer) const {
  Load* load = this->problem->pd_pattern->create_load(this->problem->load_context);
  // 局部丢弃式写缓冲: 调用方传入非空时, 本 Load 的时间窗重算只落该缓冲、不污染共享冻结表
  load->store_buffer = store_buffer;
  for (const auto order : orders) {
    this->problem->pd_pattern->insert_last_delivery(load, order);
  }
  load->change_vehicle(vehicle);
  this->problem->eval_load(load);
  load->store_buffer = nullptr;
  return load;
}

std::vector<VehicleInsertSnapshot> LoadEditor::snapshot_vehicles(
    Load* load, const VehicleResource& vehicle_resource) const {
  const Vehicle* const ori_vehicle = load->vehicle;
  std::vector<VehicleInsertSnapshot> snapshots;
  snapshots.reserve(vehicle_resource.vehicle_usages.size());
  for (int vehicle_ind = 0; vehicle_ind < vehicle_resource.len; ++vehicle_ind) {
    const VehicleUsage& usage =
        *vehicle_resource.vehicle_usages[static_cast<std::size_t>(vehicle_ind)];
    load->change_vehicle(usage.vehicle);
    this->problem->eval_load(load);
    const bool feasible = load->is_feasible();
    snapshots.push_back(VehicleInsertSnapshot{
        .vehicle = usage.vehicle,
        .vehicle_ind = vehicle_ind,
        .feasible = feasible,
        .obj_val = feasible ? load->constr_profile->obj_val : std::numeric_limits<double>::max(),
        .has_resource = usage.has_available_vehicle(),
    });
  }
  // 稳定排序: 同成本时保持 vehicle_usages 的原始顺序
  std::ranges::stable_sort(snapshots,
                           [](const VehicleInsertSnapshot& a, const VehicleInsertSnapshot& b) {
                             if (a.obj_val == b.obj_val) {
                               return a.vehicle->ind < b.vehicle->ind;
                             }
                             return a.obj_val < b.obj_val;
                           });
  load->change_vehicle(ori_vehicle);
  return snapshots;
}

const Vehicle* LoadEditor::select_current_best_vehicle(
    Load* load, const VehicleResource& vehicle_resource) const {
  const Vehicle* const ori_vehicle = load->vehicle;
  // 原车型即使已无空位也算可用; 以它为初值, 成本打平时自然保留原车型
  const Vehicle* best_vehicle = load->is_feasible() ? ori_vehicle : nullptr;
  double best_obj_val =
      load->is_feasible() ? load->constr_profile->obj_val : std::numeric_limits<double>::max();
  for (const auto& vehicle_usage : vehicle_resource.vehicle_usages) {
    if (vehicle_usage->vehicle == ori_vehicle || !vehicle_usage->has_available_vehicle()) {
      continue;
    }
    load->change_vehicle(vehicle_usage->vehicle);
    this->problem->eval_load(load);
    if (load->is_feasible() && load->constr_profile->obj_val < best_obj_val) {
      best_vehicle = vehicle_usage->vehicle;
      best_obj_val = load->constr_profile->obj_val;
    }
  }
  // 没有可用车型就退回原车型, 让调用方按 load->is_feasible() 决定是否接受
  load->change_vehicle(best_vehicle != nullptr ? best_vehicle : ori_vehicle);
  this->problem->eval_load(load);
  return best_vehicle;
}

std::optional<InsertPlan> LoadEditor::evaluate_insert(const Load* ori_load, const Segment* segment,
                                                      const VehicleResource& vehicle_resource,
                                                      CacheStoreBuffer* store_buffer,
                                                      std::uint64_t* watched_vehicles) const {
  if (watched_vehicles != nullptr) {
    *watched_vehicles = 0;
  }
  const Problem* problem = this->problem;
  // 只拷贝一次: 后续所有试插都在这份副本上进行, 每次试插后回滚
  std::unique_ptr<Load> load(problem->pd_pattern->deep_copy_load(ori_load));
  // 挂载任务私有写缓冲: 副本内的重复查找(逐槽试插同一个 node 的键)落在这里, 不触碰冻结表
  load->store_buffer = store_buffer;
  const Vehicle* ori_vehicle = load->vehicle;
  // 硬约束预检: 与插入位置无关的约束若已确定违背, 直接剪掉整个 (load, segment) 组合,
  // 省去枚举槽位与逐槽 eval
  if (problem->hc_manager->precheck(load.get(), segment, vehicle_resource) ==
      HcPrecheckVerdict::kInfeasible) {
    return std::nullopt;
  }
  const std::vector<InsertSlot> slots =
      problem->pd_pattern->enumerate_insert_slots(load.get(), segment);
  const int slot_len = static_cast<int>(slots.size());
  if (slot_len == 0) {
    return std::nullopt;
  }

  // 先按插入位置估算两端连接处的距离变化, 再按 delta dist 升序试插: 变化越小的位置越可能成为最优解
  const std::vector<std::int64_t> delta_dists =
      problem->pd_pattern->estimate_insert_delta_dists(load.get(), segment, slots);
  std::vector<int> slot_order = RangeUtils::range_vector(slot_len);
  std::ranges::stable_sort(slot_order, [&delta_dists](const int a, const int b) {
    // 同 Δdist 时保持 enumerate_insert_slots 的原始顺序
    return delta_dists[a] < delta_dists[b];
  });
  const int stop_feasible_count =
      std::max(1, static_cast<int>(std::ceil(slot_len * INSERT_FEASIBLE_STOP_RATIO)));

  const Vehicle* best_vehicle = nullptr;
  double best_obj_val = std::numeric_limits<double>::max();
  int best_slot_ind = -1;
  int feasible_count = 0;
  std::uint64_t watched = 0;

  for (const int slot_ind : slot_order) {
    // 车型每轮重置: 否则上一轮选出的车型会泄漏到本轮, 结果依赖枚举顺序
    load->reset_route_profile();
    const std::vector<Activity*> inserted =
        problem->pd_pattern->insert_segment_at(load.get(), segment, slots[slot_ind]);
    if (inserted.empty()) {
      continue;
    }
    problem->eval_load(load.get());
    // 当前车型不可行时按成本升序换车: 取第一个有空位的车型, 并记录被占满的更低成本车型
    if (load->is_infeasible()) {
      const std::vector<VehicleInsertSnapshot> snapshots =
          this->snapshot_vehicles(load.get(), vehicle_resource);
      const Vehicle* chosen = nullptr;
      for (const VehicleInsertSnapshot& snapshot : snapshots) {
        if (!snapshot.feasible) {
          continue;
        }
        // 原车型即使已无空位也算可用
        if (snapshot.has_resource || snapshot.vehicle == ori_vehicle) {
          chosen = snapshot.vehicle;
          break;
        }
        // 成本更低却因占满不可用: 记下, 该车型空出后这一格可能变可行;
        // 位图按 vehicle_ind % 64 分桶, 车型数超过 64 时多个车型共用一位
        watched |= std::uint64_t{1} << (snapshot.vehicle_ind & 63);
      }
      load->change_vehicle(chosen != nullptr ? chosen : ori_vehicle);
      problem->eval_load(load.get());
    }
    // 同分时优先保留原车型
    if (load->is_feasible()) {
      ++feasible_count;
      if (load->constr_profile->obj_val < best_obj_val) {
        best_vehicle = load->vehicle;
        best_obj_val = load->constr_profile->obj_val;
        best_slot_ind = slot_ind;
      }
    }
    // 回滚链表结构; travel_dist/travel_time/ptws 不必恢复, 每次 eval 都会全量重算
    load->remove_activities(inserted);
    load->change_vehicle(ori_vehicle);
    if (feasible_count >= stop_feasible_count) {
      break;
    }
  }

  if (watched_vehicles != nullptr) {
    *watched_vehicles = watched;
  }
  if (best_vehicle == nullptr) {
    return std::nullopt;
  }
  return InsertPlan{best_slot_ind, best_vehicle, best_obj_val};
}

Load* LoadEditor::apply_insert(const Load* ori_load, const Segment* segment, const InsertPlan& plan,
                               CacheStoreBuffer* store_buffer) const {
  const Problem* problem = this->problem;
  std::unique_ptr<Load> load(problem->pd_pattern->deep_copy_load(ori_load));
  load->store_buffer = store_buffer;
  // 重新枚举槽位: plan.slot_ind 索引的是同一条 ori_load + segment 的确定性枚举结果
  const std::vector<InsertSlot> slots =
      problem->pd_pattern->enumerate_insert_slots(load.get(), segment);
  if (plan.slot_ind < 0 || plan.slot_ind >= static_cast<int>(slots.size())) {
    return nullptr;
  }
  load->reset_route_profile();
  load->change_vehicle(plan.vehicle);
  if (problem->pd_pattern->insert_segment_at(load.get(), segment, slots[plan.slot_ind]).empty()) {
    return nullptr;
  }
  problem->eval_load(load.get());
  // 写缓冲的生命周期只到任务结束, 返回的 load 可能长期存活: 先解绑, 避免留下悬垂指针
  load->store_buffer = nullptr;
  if (load->is_infeasible()) {
    return nullptr;
  }
  return load.release();
}

Load* LoadEditor::greedy_insert(const Load* ori_load, const Segment* segment,
                                const VehicleResource& vehicle_resource,
                                CacheStoreBuffer* store_buffer) const {
  const std::optional<InsertPlan> plan =
      this->evaluate_insert(ori_load, segment, vehicle_resource, store_buffer);
  if (!plan.has_value()) {
    return nullptr;
  }
  Load* load = this->apply_insert(ori_load, segment, *plan, store_buffer);
  if (load == nullptr) {
    throw std::runtime_error("插入segment后出现无解 in function `LoadEditor::greedy_insert`");
  }
  return load;
}
