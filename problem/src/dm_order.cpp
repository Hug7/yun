/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#include "dm_order.h"

#include <memory>

#include "bd_available_vehicle.h"
#include "bd_time_window.h"
#include "bd_time_window_utils.h"
#include "c_constant.h"

Order::Order(PlanDatetimeRange* plan_datetime_range, const int ind,
             std::vector<const CargoOrder*>& cargo_orders, std::vector<long>& dim_vals,
             LabelsetValue* labelset_value, LabelsetValueBitset::UPtr labelset_value_bitset,
             Bitset::UPtr available_vehicle_bitset)
    : ind(ind) {
  this->cargo_orders = cargo_orders;
  this->pick_loc = cargo_orders[0]->pick_loc;
  this->drop_loc = cargo_orders[0]->drop_loc;
  // intersection pick and drop time windows of all cargo orders
  this->pick_time_windows.push_back(plan_datetime_range->create_default_time_window());
  this->drop_time_windows.push_back(plan_datetime_range->create_default_time_window());

  for (const auto cargo_order : cargo_orders) {
    // process pick time window
    std::vector<TimeWindow*> tmp_pick_time_windows;
    TimeWindow* cur_pick_tw = cargo_order->pick_time_window;
    for (auto& ptw : this->pick_time_windows) {
      TimeWindow* tw = TimeWindowUntils::intersection(cur_pick_tw, ptw);
      if (tw != nullptr) {
        tmp_pick_time_windows.push_back(tw);
      }
    }
    for (auto& tw : this->pick_time_windows) {
      delete tw;
    }
    this->pick_time_windows = tmp_pick_time_windows;
    // process drop time window
    std::vector<TimeWindow*> tmp_drop_time_windows;
    TimeWindow* cur_drop_tw = cargo_order->drop_time_window;
    for (auto& dtw : this->drop_time_windows) {
      TimeWindow* tw = TimeWindowUntils::intersection(cur_drop_tw, dtw);
      if (tw != nullptr) {
        tmp_drop_time_windows.push_back(tw);
      }
    }
    for (auto& tw : this->drop_time_windows) {
      delete tw;
    }
    this->drop_time_windows = tmp_drop_time_windows;
  }
  // intersection pick and drop time windows for location calendar
  Calendar* pick_calendar = this->pick_loc->work_plan->pick_calendar;
  Calendar* drop_calendar = this->drop_loc->work_plan->drop_calendar;
  std::vector<TimeWindow*> res_pick_time_windows;
  for (auto& tw : this->pick_time_windows) {
    std::vector<TimeWindow*> extend_tws = pick_calendar->intersection(tw);
    res_pick_time_windows.insert(res_pick_time_windows.end(), extend_tws.begin(), extend_tws.end());
    delete tw;
  }
  this->pick_time_windows = TimeWindowUntils::merge_time_windows(res_pick_time_windows);
  this->pick_time_windows_len = this->pick_time_windows.size();

  std::vector<TimeWindow*> res_drop_time_windows;
  for (auto& tw : this->drop_time_windows) {
    std::vector<TimeWindow*> extend_tws = drop_calendar->intersection(tw);
    res_drop_time_windows.insert(res_drop_time_windows.end(), extend_tws.begin(), extend_tws.end());
    delete tw;
  }
  this->drop_time_windows = TimeWindowUntils::merge_time_windows(res_drop_time_windows);
  this->drop_time_windows_len = this->drop_time_windows.size();

  // accumulate sub cargo order
  // 1. accumulate the dim values
  // 2. accumulate labelset value
  // 3. accumulate available vehicle bitset
  this->dim_vals = dim_vals;
  this->labelset_value = labelset_value;
  this->labelset_value_bitset = std::move(labelset_value_bitset);
  this->available_vehicle_bitset = std::move(available_vehicle_bitset);
  const int dim_vals_len = this->dim_vals.size();
  for (const auto& cargo_order : cargo_orders) {
    for (const auto& sub_order : cargo_order->sub_orders) {
      // accumulate the dim values
      for (int u = 0; u < dim_vals_len; u++) {
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
  for (auto& tw : this->pick_time_windows) {
    delete tw;
  }
  this->pick_time_windows.clear();
  for (auto& tw : this->drop_time_windows) {
    delete tw;
  }
  this->drop_time_windows.clear();
  delete this->labelset_value;
}

std::vector<TimeWindow*> Order::copy_pick_time_windows() const {
  std::vector<TimeWindow*> copy_tws(this->pick_time_windows_len);
  for (int u = 0; u < this->pick_time_windows_len; ++u) {
    copy_tws[u] = new TimeWindow(this->pick_time_windows[u]);
  }
  return copy_tws;
}

std::vector<TimeWindow*> Order::copy_drop_time_windows() const {
  std::vector<TimeWindow*> copy_tws(this->drop_time_windows_len);
  for (int u = 0; u < this->drop_time_windows_len; ++u) {
    copy_tws[u] = new TimeWindow(this->drop_time_windows[u]);
  }
  return copy_tws;
}
