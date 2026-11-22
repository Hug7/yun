/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#include "pdm_order.h"

#include <format>
#include <memory>
#include <stdexcept>
#include <utility>

#include "bdm_available_vehicle.h"
#include "bdm_time_window.h"
#include "bdm_time_window_utils.h"
#include "c_constant.h"
#include "pdm_time_window_pattern.h"

namespace {
/**
 * @brief 用单个时间窗裁剪一组时间窗
 */
std::vector<TimeWindow> intersect_windows(const std::vector<TimeWindow>& base,
                                          const TimeWindow& tw) {
  std::vector<TimeWindow> res;
  res.reserve(base.size());
  for (const auto& cur_tw : base) {
    if (const auto hit = TimeWindowUntils::intersection(cur_tw, tw); hit.has_value()) {
      res.push_back(hit.value());
    }
  }
  return res;
}
}  // namespace

// ====== implement of Order ======
Order::Order(const PlanDatetimeRange* plan_datetime_range, int ind,
             std::vector<CargoOrder*>& cargo_orders, std::vector<std::int64_t>& dim_vals,
             LabelsetValue* labelset_value, LabelsetValueBitset::UPtr labelset_value_bitset,
             Bitset::UPtr available_vehicle_bitset, TimeWindowPatternRegistry* pattern_registry)
    : ind(ind) {
  this->cargo_orders = cargo_orders;
  this->pick_loc = cargo_orders[0]->pick_loc;
  this->drop_loc = cargo_orders[0]->drop_loc;
  // intersection pick and drop time windows of all cargo orders
  this->pick_time_windows.push_back(plan_datetime_range->create_default_time_window());
  this->drop_time_windows.push_back(plan_datetime_range->create_default_time_window());

  for (const auto cargo_order : cargo_orders) {
    // 时间窗缺失属于数据错误, 在此显式报错而不是解引用空指针
    if (!cargo_order->pick_time_window.has_value() || !cargo_order->drop_time_window.has_value()) {
      throw std::runtime_error(
          std::format("CargoOrder {} 缺少提货或卸货时间窗", cargo_order->code));
    }
    this->pick_time_windows =
        intersect_windows(this->pick_time_windows, cargo_order->pick_time_window.value());
    this->drop_time_windows =
        intersect_windows(this->drop_time_windows, cargo_order->drop_time_window.value());
  }
  // intersection pick and drop time windows for location calendar
  const Calendar* pick_calendar = this->pick_loc->work_plan->pick_calendar;
  if (pick_calendar != nullptr) {
    std::vector<TimeWindow> res_pick_time_windows;
    for (const auto& tw : this->pick_time_windows) {
      const std::vector<TimeWindow> extend_tws = pick_calendar->intersection(tw);
      res_pick_time_windows.insert(res_pick_time_windows.end(), extend_tws.begin(),
                                   extend_tws.end());
    }
    this->pick_time_windows = TimeWindowUntils::merge_time_windows(res_pick_time_windows);
  }
  const Calendar* drop_calendar = this->drop_loc->work_plan->drop_calendar;
  if (drop_calendar != nullptr) {
    std::vector<TimeWindow> res_drop_time_windows;
    for (const auto& tw : this->drop_time_windows) {
      const std::vector<TimeWindow> extend_tws = drop_calendar->intersection(tw);
      res_drop_time_windows.insert(res_drop_time_windows.end(), extend_tws.begin(),
                                   extend_tws.end());
    }
    this->drop_time_windows = TimeWindowUntils::merge_time_windows(res_drop_time_windows);
  }

  // 时间窗已最终确定, 登记模式 id 供节点时间窗缓存构造键; 同一内容的不同订单共享一个 id
  this->pick_tw_pattern_id = pattern_registry->intern(this->pick_time_windows);
  this->drop_tw_pattern_id = pattern_registry->intern(this->drop_time_windows);

  // accumulate sub cargo order
  // 1. accumulate the dim values
  // 2. accumulate labelset value
  // 3. accumulate available vehicle bitset
  this->dim_vals = dim_vals;
  this->labelset_value = labelset_value;
  this->labelset_value_bitset = std::move(labelset_value_bitset);
  this->available_vehicle_bitset = std::move(available_vehicle_bitset);
  const size_t dim_vals_len = this->dim_vals.size();
  for (const auto& cargo_order : cargo_orders) {
    for (const auto& sub_order : cargo_order->sub_orders) {
      // accumulate the dim values
      for (size_t u = 0; u < dim_vals_len; u++) {
        this->dim_vals[u] += sub_order->dim_vals[u];
      }
      // accumulate labelset value
      this->labelset_value->merge(sub_order->labelset_value);
      this->labelset_value_bitset->merge(sub_order->labelset_value_bitset);
    }
  }

  // process pick and drop work time
  this->pick_work_time = cargo_orders[0]->pick_loc->work_plan->work_effect->get_work_time(
      ActivityType::PICK, this->dim_vals);
  this->drop_work_time = cargo_orders[0]->drop_loc->work_plan->work_effect->get_work_time(
      ActivityType::DROP, this->dim_vals);

  // intersection available vehicle bitset
  for (const auto cargo_order : cargo_orders) {
    this->available_vehicle_bitset->call_intersection(
        cargo_order->pick_loc->available_vehicle->vehicle_bitset);
    this->available_vehicle_bitset->call_intersection(
        cargo_order->drop_loc->available_vehicle->vehicle_bitset);
  }
}

Order::~Order() {
  this->cargo_orders.clear();
  this->pick_time_windows.clear();
  this->drop_time_windows.clear();
  delete this->labelset_value;
}

// ====== implement of OrderFactory ======
Order* OrderFactory::creat_order(std::vector<CargoOrder*>& cargo_orders, const int ind,
                                 const PlanDatetimeRange* plan_datetime_range,
                                 const Scenario* scenario,
                                 TimeWindowPatternRegistry* pattern_registry) {
  auto dim_vals = scenario->dim_manager->empty_dim_values();
  auto labelset_value = scenario->label_manager->order_labelset->empty_labelset_value();
  auto labelset_value_bitset =
      scenario->label_manager->order_labelset->empty_labelset_value_bitset();
  auto available_vehicle_bitset = scenario->carrier_manager->full_vehicle_bitset();

  return new Order(plan_datetime_range, ind, cargo_orders, dim_vals, labelset_value,
                   std::move(labelset_value_bitset), std::move(available_vehicle_bitset),
                   pattern_registry);
}

Order* OrderFactory::creat_tmp_order(std::vector<CargoOrder*>& cargo_orders,
                                     const PlanDatetimeRange* plan_datetime_range,
                                     const Scenario* scenario,
                                     TimeWindowPatternRegistry* pattern_registry) {
  return creat_order(cargo_orders, -1, plan_datetime_range, scenario, pattern_registry);
}
