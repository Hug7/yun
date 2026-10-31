/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ranges>

#include "pdm_func_timing.h"
#include "visual_manager.h"

namespace {
/**
 * @brief 单个车次的货单数(按 Order::cargo_orders 展开计数)
 */
int count_cargo_orders(Load* load) {
  int n = 0;
  for (const Order* order : load->get_orders()) {
    n += static_cast<int>(order->cargo_orders.size());
  }
  return n;
}
/**
 * @brief 转换规划结果
 * @param loads 车次集合
 * @param unassigned_orders 未配送订单集合
 * @param func_timings 函数记数列表
 * @return 规划结果全局汇总
 */
VisualPlanSummary summarize_plan(const std::vector<Load*>& loads,
                                 const std::vector<const Order*>& unassigned_orders,
                                 const FuncTiming::VecUPtr& func_timings) {
  VisualPlanSummary summary;
  // 未配送货单数(按 Order::cargo_orders 展开)
  for (const Order* order : unassigned_orders) {
    summary.unassigned_cargo_order_count += static_cast<int>(order->cargo_orders.size());
  }
  // 全局汇总 + 承运商+车型 分组(按约定不可行 Load 不出现, 直接求和)
  std::map<std::pair<std::string, std::string>, VisualCarrierVehicleSummary> group_map;
  for (Load* load : loads) {
    const auto& cp = *load->constr_profile;
    const int cargo_count = count_cargo_orders(load);
    summary.total_load_count += 1;
    summary.total_cargo_order_count += cargo_count;
    summary.total_travel_distance += load->get_total_travel_dist();
    summary.total_travel_time += load->get_total_travel_time();
    summary.total_cost += cp.total_cost;
    summary.total_penalty += cp.total_soft_penalty;
    summary.total_objective += cp.obj_val;

    if (load->vehicle != nullptr && load->vehicle->carrier != nullptr &&
        load->vehicle->vehicle_model != nullptr) {
      const auto key = std::make_pair(std::string(load->vehicle->carrier->code),
                                      std::string(load->vehicle->vehicle_model->code));
      auto& g = group_map[key];
      g.carrier_code = key.first;
      g.vehicle_model_code = key.second;
      g.cargo_order_count += cargo_count;
      g.load_count += 1;
      g.total_travel_distance += load->get_total_travel_dist();
      g.total_travel_time += load->get_total_travel_time();
      g.total_cost += cp.total_cost;
      g.total_penalty += cp.total_soft_penalty;
      g.total_objective += cp.obj_val;
    }
  }
  // 总货单数 = 已配送 + 未配送
  summary.total_cargo_order_count += summary.unassigned_cargo_order_count;
  for (auto& value : group_map | std::views::values) {
    summary.carrier_vehicle_summaries.emplace_back(std::move(value));
  }
  // 统计时间
  for (const auto& func_timing : func_timings) {
    if (func_timing->func_code == "solve") {
      summary.start_date_time = func_timing->start_date_time;
      summary.end_date_time = func_timing->end_date_time;
      summary.running_time_second = func_timing->elapsed_second();
    } else {
      VisualFuncTiming visual_func_timing;
      visual_func_timing.func_code = func_timing->func_code;
      visual_func_timing.start_date_time = func_timing->start_date_time;
      visual_func_timing.end_date_time = func_timing->end_date_time;
      visual_func_timing.elapsed_second = func_timing->elapsed_second();
      summary.func_timings.push_back(std::move(visual_func_timing));
    }
  }

  return summary;
}
}  // namespace

// ====== implement of Load VisualManager ======

void VisualManager::localization_plan_result(const std::vector<Load*>& loads,
                                             const std::vector<const Order*>& unassigned_orders,
                                             const FuncTiming::VecUPtr& func_timings,
                                             const std::string& target_dir) const {
  // 目标文件夹
  const std::string target_dir_path = this->resolve_target_dir(target_dir);
  // 汇总 visual plan summary
  const VisualPlanSummary visual_plan_summary =
      summarize_plan(loads, unassigned_orders, func_timings);
  // 将 visual plan summary 写入json
  this->visual_plan_summary_to_json(visual_plan_summary, target_dir_path);
  // 将 load 转化为 visual load
  const auto visual_loads = this->loads_to_visual_loads(loads);
  // load车次维度 写入json
  this->visual_load_to_json(visual_loads, target_dir_path);
  // 将 load 转化为 visual load location
  const auto visual_load_location = this->loads_to_visual_load_locations(visual_loads, loads);
  // load站点维度 写入json
  this->visual_load_location_to_json(visual_load_location, target_dir_path);
  // 未指派订单 写入json
  this->unassigned_order_to_json(unassigned_orders, target_dir_path);
}

void VisualManager::localization_plan_result(const std::vector<Load*>& loads,
                                             const std::vector<const Order*>& unassigned_orders,
                                             const FuncTiming::VecUPtr& func_timings) const {
  this->localization_plan_result(loads, unassigned_orders, func_timings, "");
}
