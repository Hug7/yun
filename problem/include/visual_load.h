/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <string>
#include <utility>
#include <vector>

struct VisualDimension {
  std::string dimension_code;

  double dimension_value;

  VisualDimension(std::string dimension_code, const double dimension_value)
      : dimension_code(std::move(dimension_code)), dimension_value(dimension_value) {}
};

struct VisualLabel {
  std::string label_code;

  std::vector<std::string> label_values;

  VisualLabel(std::string label_code, std::vector<std::string> label_values)
      : label_code(std::move(label_code)), label_values(std::move(label_values)) {};
};

struct VisualCargoOrder {
  std::string cargo_order_code;

  std::string cargo_sub_order_code;

  int quantity{0};

  std::vector<VisualDimension> order_dimensions;

  std::vector<VisualLabel> order_labels;

  VisualCargoOrder() = default;
};

struct VisualLoad {
  std::string load_code;

  std::string carrier_code;

  std::string vehicle_model_code;

  std::vector<VisualDimension> vehicle_load_dimensions;

  std::vector<VisualDimension> load_dimensions;

  std::vector<VisualDimension> loading_rate_dimensions;

  std::string loading_dimension_code;

  double loading_rate{0.0};

  long total_driving_distance_meter{0};

  long total_driving_time_second{0};

  long total_work_time_second{0};

  long total_wait_time_second{0};

  long total_cost_time_second{0};

  double soft_penalty_value{0};

  double cost_value{0};

  double object_value{0};

  std::vector<VisualLabel> pick_location_labels;

  std::vector<VisualLabel> drop_location_labels;

  std::vector<VisualLabel> order_labels;

  std::vector<std::string> location_sequences;

  std::string plan_earliest_start_time;

  long plan_earliest_start_timestamp{0};

  std::string plan_earliest_end_time;

  long plan_earliest_end_timestamp{0};

  std::string plan_latest_start_time;

  long plan_latest_start_timestamp{0};

  std::string plan_latest_end_time;

  long plan_latest_end_timestamp{0};

  VisualLoad() = default;
};

struct VisualLoadLocation {
  std::string load_code;

  std::string carrier_code;

  std::string vehicle_model_code;

  std::string activity_type;

  std::vector<VisualCargoOrder> cargo_orders;

  std::vector<VisualLabel> location_labels;

  std::vector<VisualLabel> order_labels;

  long prev_driving_distance_meter{0};

  long prev_driving_time_second{0};

  long wait_time_second{0};

  long work_time_second{0};

  std::string plan_earliest_arrive_time;

  long plan_earliest_arrive_timestamp{0};

  std::string plan_earliest_start_work_time;

  long plan_earliest_start_work_timestamp{0};

  std::string plan_earliest_depart_time;

  long plan_earliest_depart_timestamp{0};

  std::string plan_latest_arrive_time;

  long plan_latest_arrive_timestamp{0};

  std::string plan_latest_start_work_time;

  long plan_latest_start_work_timestamp{0};

  std::string plan_latest_depart_time;

  long plan_latest_depart_timestamp{0};

  VisualLoadLocation() = default;
};

struct VisualInfeasibleCargoOrder {
  std::vector<VisualCargoOrder> cargo_orders;

  VisualInfeasibleCargoOrder() = default;
};

namespace VisualJson {
/**
 * @brief 将车次集合序列化为json文本
 * @param visual_loads 车次集合
 * @return json文本, 结构见 resources/template/output/Loads.json
 */
std::string dumps(const std::vector<VisualLoad>& visual_loads);
}  // namespace VisualJson
