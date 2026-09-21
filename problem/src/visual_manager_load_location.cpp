/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#include <spdlog/spdlog.h>

#include <algorithm>
#include <format>
#include <fstream>
#include <magic_enum/magic_enum.hpp>
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
 * @brief 解码标签集合值位图
 * @param labelset 标签集合
 * @param labelset_value_bitset 标签集合值位图
 * @return 标签集合
 */
std::vector<VisualLabel> decode_labels(const Labelset* labelset,
                                       const LabelsetValueBitset* labelset_value_bitset) {
  std::vector<VisualLabel> labels;
  if (labelset_value_bitset == nullptr) {
    return labels;
  }
  for (int la = 0; la < labelset->len; ++la) {
    const auto label = labelset->labels[la];
    std::vector<std::string> label_values;
    for (int t = 0; t < label->len; ++t) {
      if (labelset_value_bitset->bitsets[la]->test(t)) {
        label_values.emplace_back(label->values[t]->value);
      }
    }
    labels.emplace_back(label->code, label_values);
  }
  return labels;
}
/**
 * @brief 解码多个订单的标签集合值位图, 取并集
 * @param labelset 标签集合
 * @param orders 订单集合
 * @return 标签集合
 */
std::vector<VisualLabel> decode_orders_labels(const Labelset* labelset,
                                              const std::vector<const Order*>& orders) {
  std::vector<VisualLabel> labels;
  for (int la = 0; la < labelset->len; ++la) {
    const auto label = labelset->labels[la];
    std::vector<std::string> label_values;
    for (int t = 0; t < label->len; ++t) {
      for (const auto order : orders) {
        if (order->labelset_value_bitset != nullptr &&
            order->labelset_value_bitset->bitsets[la]->test(t)) {
          label_values.emplace_back(label->values[t]->value);
          break;
        }
      }
    }
    labels.emplace_back(label->code, label_values);
  }
  return labels;
}
/**
 * @brief 维度值转为可视化维度集合
 * @param dim_manager 维度管理器
 * @param dim_vals 维度值
 * @return 可视化维度集合
 */
std::vector<VisualDimension> to_visual_dimensions(const DimensionManager* dim_manager,
                                                  const std::vector<long>& dim_vals) {
  const int dim_len = std::min(dim_manager->len, static_cast<int>(dim_vals.size()));
  std::vector<VisualDimension> dimensions;
  dimensions.reserve(dim_len);
  for (int d = 0; d < dim_len; ++d) {
    dimensions.emplace_back(
        dim_manager->dimensions[d]->code,
        NumUtil::unscale_from_long(dim_vals[d], dim_manager->dimensions[d]->precision));
  }
  return dimensions;
}
/**
 * @brief 填充站点上的订单明细
 * @param scenario 场景
 * @param visual_load_location 车次站点可视化数据
 * @param node 车次的节点
 */
void fill_cargo_orders(const Scenario* scenario, VisualLoadLocation& visual_load_location,
                       const Node* node) {
  const auto dim_manager = scenario->dim_manager;
  const auto order_labelset = scenario->label_manager->order_labelset;
  for (auto activity = node->first.get(); activity != nullptr; activity = activity->next.get()) {
    for (const auto cargo_order : activity->order->cargo_orders) {
      for (const auto sub_order : cargo_order->sub_orders) {
        VisualCargoOrder visual_cargo_order;
        visual_cargo_order.cargo_order_code = cargo_order->code;
        visual_cargo_order.cargo_sub_order_code = sub_order->code;
        visual_cargo_order.quantity = sub_order->qty;
        visual_cargo_order.order_dimensions =
            to_visual_dimensions(dim_manager, sub_order->dim_vals);
        visual_cargo_order.pick_location_code = cargo_order->pick_loc->code;
        visual_cargo_order.drop_location_code = cargo_order->drop_loc->code;
        visual_cargo_order.order_labels =
            decode_labels(order_labelset, sub_order->labelset_value_bitset.get());
        visual_load_location.cargo_orders.emplace_back(std::move(visual_cargo_order));
      }
    }
  }
}
/**
 * @brief 填充站点标签和站点上订单的标签
 * @param scenario 场景
 * @param visual_load_location 车次站点可视化数据
 * @param node 车次的节点
 */
void fill_labels(const Scenario* scenario, VisualLoadLocation& visual_load_location,
                 const Node* node) {
  const auto label_manager = scenario->label_manager;
  // 站点标签
  visual_load_location.location_labels =
      decode_labels(label_manager->location_labelset, node->loc->labelset_value_bitset.get());
  // 站点上订单的标签
  visual_load_location.order_labels =
      decode_orders_labels(label_manager->order_labelset, node->get_orders());
}
/**
 * @brief 填充行驶信息、等待和作业时长、计划时间
 * @details 计划时间取节点的第一段计划时间窗, 开始作业时间 = 离开时间 - 作业时长
 * @param visual_load_location 车次站点可视化数据
 * @param node 车次的节点
 * @param time_window_plan 计划时间
 */
void fill_plan_time(VisualLoadLocation& visual_load_location, const Node* node,
                    const TimeWindowPlan::UPtr& time_window_plan) {
  visual_load_location.prev_driving_distance_meter = node->travel_dist;
  visual_load_location.prev_driving_time_second = node->travel_time;
  visual_load_location.work_time_second = node->get_work_time();
  if (node->ptws.empty()) {
    return;
  }
  visual_load_location.wait_time_second = time_window_plan->wait_time;
  visual_load_location.plan_earliest_arrive_timestamp = time_window_plan->early_arr;
  visual_load_location.plan_earliest_depart_timestamp = time_window_plan->early_depart;
  visual_load_location.plan_earliest_start_work_timestamp =
      time_window_plan->early_depart - visual_load_location.work_time_second;
  visual_load_location.plan_latest_arrive_timestamp = time_window_plan->late_arr;
  visual_load_location.plan_latest_depart_timestamp = time_window_plan->late_depart;
  visual_load_location.plan_latest_start_work_timestamp =
      time_window_plan->late_depart - visual_load_location.work_time_second;

  visual_load_location.plan_earliest_arrive_time = TimeParse::format_sec_to_tm(
      visual_load_location.plan_earliest_arrive_timestamp, TimeParse::fmt_yyyymmddhhmm_3);
  visual_load_location.plan_earliest_start_work_time = TimeParse::format_sec_to_tm(
      visual_load_location.plan_earliest_start_work_timestamp, TimeParse::fmt_yyyymmddhhmm_3);
  visual_load_location.plan_earliest_depart_time = TimeParse::format_sec_to_tm(
      visual_load_location.plan_earliest_depart_timestamp, TimeParse::fmt_yyyymmddhhmm_3);
  visual_load_location.plan_latest_arrive_time = TimeParse::format_sec_to_tm(
      visual_load_location.plan_latest_arrive_timestamp, TimeParse::fmt_yyyymmddhhmm_3);
  visual_load_location.plan_latest_start_work_time = TimeParse::format_sec_to_tm(
      visual_load_location.plan_latest_start_work_timestamp, TimeParse::fmt_yyyymmddhhmm_3);
  visual_load_location.plan_latest_depart_time = TimeParse::format_sec_to_tm(
      visual_load_location.plan_latest_depart_timestamp, TimeParse::fmt_yyyymmddhhmm_3);
}
/**
 * @brief 拼装单个车次站点的可视化数据
 * @param scenario 场景
 * @param visual_load 可视化车次
 * @param time_window_plan 计划时间
 * @param node 车次的节点
 * @return 车次站点可视化数据
 */
VisualLoadLocation build_visual_load_location(const Scenario* scenario,
                                              const VisualLoad& visual_load,
                                              const TimeWindowPlan::UPtr& time_window_plan,
                                              const Node* node) {
  VisualLoadLocation visual_load_location;
  visual_load_location.load_code = visual_load.load_code;
  visual_load_location.carrier_code = visual_load.carrier_code;
  visual_load_location.vehicle_model_code = visual_load.vehicle_model_code;
  visual_load_location.location_code = node->loc->code;
  visual_load_location.activity_type = std::string(magic_enum::enum_name(node->activity_type));
  // 站点上的订单明细
  fill_cargo_orders(scenario, visual_load_location, node);
  // 站点标签、订单标签
  fill_labels(scenario, visual_load_location, node);
  // 行驶信息、等待和作业时长、计划时间
  fill_plan_time(visual_load_location, node, time_window_plan);
  return visual_load_location;
}
}  // namespace

std::vector<VisualLoadLocation> VisualManager::loads_to_visual_load_locations(
      const std::vector<VisualLoad>& visual_loads, const std::vector<Load*>& loads) const {
  // 拼装visual load location
  std::vector<VisualLoadLocation> visual_load_locations;
  for (size_t l = 0; l < loads.size(); ++l) {
    const auto load = loads[l];
    const auto nodes = load->unfold_node_linked();
    const int node_len = static_cast<int>(nodes.size());
    visual_load_locations.reserve(visual_load_locations.size() + nodes.size());
    // 计划时间列表
    TimeWindowPlan::VecUPtr time_window_plans = load->infer_time_window_plans();
    for (int u = 0; u < node_len; ++u) {
      const auto& node = nodes[u];
      if (node->loc->is_default()) {
        continue;
      }
      visual_load_locations.emplace_back(
          build_visual_load_location(this->scenario, visual_loads[l], time_window_plans[u], node));
    }
  }

  return visual_load_locations;
}

void VisualManager::visual_load_location_to_json(const std::vector<VisualLoadLocation>& visual_load_locations,
                                    const std::string& target_dir_path) const {
  // 写出json文件
  const std::string file_path = target_dir_path + "/" + LOAD_LOCATIONS_FILE_NAME;
  std::ofstream file(file_path);
  if (!file.is_open()) {
    this->logger->error(std::format("创建文件 {} 失败!", file_path));
    return;
  }
  file << VisualJson::dumps(visual_load_locations);
  file.close();
}
