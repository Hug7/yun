/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#include <spdlog/spdlog.h>

#include <filesystem>
#include <format>
#include <fstream>
#include <ranges>
#include <string>
#include <utility>
#include <vector>

#include "c_numerical_utils.h"
#include "c_time_utils.h"
#include "visual_manager.h"
#include "visual_plan_result_load.h"
#include "visual_schema.h"

// ====== implement of Load VisualManager ======
namespace {
/**
 * @brief 填充车辆装载属性、峰值负载、装载率
 * @param dim_manager 维度管理器
 * @param visual_load 车次可视化数据
 * @param load 车次
 */
void fill_dimensions(const DimensionManager* dim_manager, VisualLoad& visual_load,
                     const Load* load) {
  const auto& dimensions = dim_manager->dimensions;
  const int dim_len = dim_manager->len;
  for (int d = 0; d < dim_len; ++d) {
    visual_load.vehicle_load_dimensions.emplace_back(
        dimensions[d]->code, NumUtil::unscale_from_long(load->vehicle->vehicle_model->dim_vals[d],
                                                        dimensions[d]->precision));
    visual_load.load_dimensions.emplace_back(
        dimensions[d]->code, NumUtil::unscale_from_long(load->route_profile->peak_load_dims[d],
                                                        dimensions[d]->precision));
    const double cur_loading_rate =
        NumUtil::decimal_places(static_cast<double>(load->route_profile->peak_load_dims[d]) /
                                    static_cast<double>(load->vehicle->vehicle_model->dim_vals[d]),
                                4);
    visual_load.loading_rate_dimensions.emplace_back(dimensions[d]->code, cur_loading_rate);
    // 更新最大装载率
    if (cur_loading_rate >= visual_load.loading_rate) {
      visual_load.loading_dimension_code = dimensions[d]->code;
      visual_load.loading_rate = cur_loading_rate;
    }
  }
}
/**
 * @brief 填充行驶时长、作业时长、站点序列
 * @param visual_load 车次可视化数据
 * @param nodes 车次的节点链
 */
void fill_route_stats(VisualLoad& visual_load, const std::vector<Node*>& nodes) {
  for (const auto& node : nodes) {
    visual_load.total_driving_time_second += node->travel_time;
    visual_load.total_work_time_second += node->get_work_time();
    if (!node->loc->is_default()) {
      visual_load.location_sequences.emplace_back(node->loc->code);
    }
  }
}
/**
 * @brief 填充站点标签(提货、卸货)和订单标签
 * @param label_manager 标签管理器
 * @param visual_load 车次可视化数据
 * @param load 车次
 */
void fill_labels(const LabelManager* label_manager, VisualLoad& visual_load, Load* load) {
  // 站点标签-提货、卸货
  const auto pick_loc_labelset_value_bitset = load->get_pick_loc_labelset_value_bitset();
  const auto drop_loc_labelset_value_bitset = load->get_drop_loc_labelset_value_bitset();
  const auto location_labelset = label_manager->location_labelset;
  for (int la = 0; la < location_labelset->len; ++la) {
    const auto loc_label = location_labelset->labels[la];
    std::vector<std::string> cur_pick_loc_label_values;
    std::vector<std::string> cur_drop_loc_label_values;
    for (int t = 0; t < loc_label->len; ++t) {
      if (pick_loc_labelset_value_bitset->bitsets[la]->test(t)) {
        cur_pick_loc_label_values.emplace_back(loc_label->values[t]->value);
      }
      if (drop_loc_labelset_value_bitset->bitsets[la]->test(t)) {
        cur_drop_loc_label_values.emplace_back(loc_label->values[t]->value);
      }
    }
    visual_load.pick_location_labels.emplace_back(loc_label->code, cur_pick_loc_label_values);
    visual_load.drop_location_labels.emplace_back(loc_label->code, cur_drop_loc_label_values);
  }
  // 订单标签
  const auto order_labelset_value_bitset = load->get_order_labelset_value_bitset();
  const auto order_labelset = label_manager->order_labelset;
  for (int la = 0; la < order_labelset->len; ++la) {
    const auto order_label = order_labelset->labels[la];
    std::vector<std::string> cur_order_label_values;
    for (int t = 0; t < order_label->len; ++t) {
      if (order_labelset_value_bitset->bitsets[la]->test(t)) {
        cur_order_label_values.emplace_back(order_label->values[t]->value);
      }
    }
    visual_load.order_labels.emplace_back(order_label->code, cur_order_label_values);
  }
}
/**
 * @brief 反向推导时间窗, 填充等待时长和计划起止时间
 * @param visual_load 车次可视化数据
 * @param load 车次
 */
void fill_plan_time(VisualLoad& visual_load, const Load* load) {
  const auto time_window_plans = load->infer_time_window_plans();
  const int node_len = static_cast<int>(time_window_plans.size());

  for (const auto& twp : time_window_plans) {
    visual_load.total_wait_time_second += twp->wait_time;
  }

  visual_load.plan_earliest_start_timestamp = time_window_plans[0]->early_arr;
  visual_load.plan_earliest_start_time = TimeParse::format_sec_to_tm(
      visual_load.plan_earliest_start_timestamp, TimeParse::fmt_yyyymmddhhmm_3);

  visual_load.plan_earliest_end_timestamp = time_window_plans[node_len - 1]->early_depart;
  visual_load.plan_earliest_end_time = TimeParse::format_sec_to_tm(
      visual_load.plan_earliest_end_timestamp, TimeParse::fmt_yyyymmddhhmm_3);

  visual_load.plan_latest_start_timestamp = time_window_plans[0]->late_arr;
  visual_load.plan_latest_start_time = TimeParse::format_sec_to_tm(
      visual_load.plan_latest_start_timestamp, TimeParse::fmt_yyyymmddhhmm_3);

  visual_load.plan_latest_end_timestamp = time_window_plans[node_len - 1]->late_depart;
  visual_load.plan_latest_end_time = TimeParse::format_sec_to_tm(
      visual_load.plan_latest_end_timestamp, TimeParse::fmt_yyyymmddhhmm_3);
}
/**
 * @brief 拼装单个车次的可视化数据
 * @param scenario 场景
 * @param load 车次
 * @param load_ind 车次序号
 * @return 车次可视化数据
 */
VisualLoad build_visual_load(const Scenario* scenario, Load* load, const size_t load_ind) {
  VisualLoad visual_load;
  visual_load.load_code = std::format("F{:04d}", load_ind);
  visual_load.carrier_code = load->vehicle->carrier->code;
  visual_load.vehicle_model_code = load->vehicle->vehicle_model->code;
  // 车辆装载属性、峰值负载、装载率
  fill_dimensions(scenario->dim_manager, visual_load, load);
  // 行驶距离、软约束惩罚、成本、目标值
  visual_load.total_driving_distance_meter = load->get_total_dist();
  visual_load.soft_penalty_value = load->constr_profile->total_soft_penalty;
  visual_load.cost_value = load->constr_profile->total_cost;
  visual_load.object_value = load->constr_profile->obj_val;

  const auto nodes = load->unfold_node_linked();
  // 行驶时长、作业时长、站点序列
  fill_route_stats(visual_load, nodes);
  // 站点标签、订单标签
  fill_labels(scenario->label_manager, visual_load, load);
  // 等待时长、计划起止时间
  fill_plan_time(visual_load, load);
  return visual_load;
}
}  // namespace

std::vector<VisualLoad> VisualManager::loads_to_visual_loads(const std::vector<Load*>& loads) const {
  // 拼装visual load
  std::vector<VisualLoad> visual_loads;
  visual_loads.reserve(loads.size());
  for (size_t l = 0; l < loads.size(); ++l) {
    visual_loads.emplace_back(build_visual_load(this->scenario, loads[l], l));
  }

  return visual_loads;
}

void VisualManager::visual_load_to_json(const std::vector<VisualLoad>& visual_loads,
                                 const std::string& target_dir_path) const {

  // 写出json文件
  const std::string file_path = target_dir_path + "/" + LOADS_FILE_NAME;
  std::ofstream file(file_path);
  if (!file.is_open()) {
    this->logger->error(std::format("创建文件 {} 失败!", file_path));
    return;
  }
  file << VisualJson::dumps(visual_loads);
  file.close();
}
