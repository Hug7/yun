/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <string>
#include <utility>
#include <vector>

class Load;
class Order;

struct VisualInfeasibleReason {
  std::string reason_code;

  std::string constraint_code;

  std::string message_cn;

  std::string message_en;

  VisualInfeasibleReason(std::string reason_code, std::string constraint_code,
                         std::string message_cn, std::string message_en)
      : reason_code(std::move(reason_code)),
        constraint_code(std::move(constraint_code)),
        message_cn(std::move(message_cn)),
        message_en(std::move(message_en)) {};
};

struct VisualVehicleInfeasibleReason {
  std::string carrier_code;

  std::string vehicle_model_code;

  std::vector<VisualInfeasibleReason> infeasible_reasons;

  VisualVehicleInfeasibleReason() = default;
};

struct VisualInfeasibleCargoOrder {
  std::vector<std::string> cargo_order_codes;

  std::vector<VisualInfeasibleReason> common_infeasible_reasons;

  std::vector<VisualVehicleInfeasibleReason> vehicle_infeasible_reasons;

  VisualInfeasibleCargoOrder() = default;
};

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

  std::string pick_location_code;

  std::string drop_location_code;

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

  std::string location_code;

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

struct VisualUnassignedCargoOrder {
  std::vector<VisualCargoOrder> cargo_orders;

  VisualUnassignedCargoOrder() = default;
};

struct VisualFuncTiming {
  /**
   * @brief 函数编码
   */
  std::string func_code;
  /**
   * @brief 开始时间
   * @details 格式 %Y%m%d%H%M%S
   */
  std::string start_date_time;
  /**
   * @brief 结束时间
   * @details 格式 %Y%m%d%H%M%S
   */
  std::string end_date_time;
  /**
   * @brief 函数耗时
   * @details = end_date_time - start_date_time，单位(s)
   */
  long elapsed_second{0};

  VisualFuncTiming() = default;
};


struct VisualCarrierVehicleSummary {
  /**
   * @brief 承运商编码
   */
  std::string carrier_code;
  /**
   * @brief 车型编码
   */
  std::string vehicle_model_code;
  /**
   * @brief cargo order 数量
   */
  int cargo_order_count{0};
  /**
   * @brief load 数量
   */
  int load_count{0};
  /**
   * @brief 总行驶距离(m)
   */
  long total_travel_distance{0};
  /**
   * @brief 总行驶耗时(s)
   */
  long total_travel_time{0};
  /**
   * @brief 总成本(= LoadConstrProfile::total_cost)
   */
  double total_cost{0.0};
  /**
   * @brief 总惩罚值(仅 soft = LoadConstrProfile::total_soft_penalty)
   */
  double total_penalty{0.0};
  /**
   * @brief 总目标值(= LoadConstrProfile::obj_val = hard + soft + cost)
   */
  double total_objective{0.0};

  VisualCarrierVehicleSummary() = default;
};

/**
 * @brief 规划结果汇总
 */
struct VisualPlanSummary {
  /**
   * @brief 未分配的 cargo order 数
   */
  int unassigned_cargo_order_count{0};
  /**
   * @brief 总 cargo order 数
   */
  int total_cargo_order_count{0};
  /**
   * @brief 总 load 数
   */
  int total_load_count{0};
  /**
   * @brief 总行驶距离(m)
   */
  long total_travel_distance{0};
  /**
   * @brief 总行驶耗时(s)
   */
  long total_travel_time{0};
  /**
   * @brief 总成本(= Σ LoadConstrProfile::total_cost)
   */
  double total_cost{0.0};
  /**
   * @brief 总惩罚值(仅 soft = Σ total_soft_penalty)
   */
  double total_penalty{0.0};
  /**
   * @brief 总目标值(= Σ obj_val = 总成本 + 总惩罚值)
   */
  double total_objective{0.0};
  /**
   * @brief 承运商+车型 维度分组汇总
   */
  std::vector<VisualCarrierVehicleSummary> carrier_vehicle_summaries;
  /**
   * @brief 开始求解时间节点
   * @details 格式 %Y%m%d%H%M%S
   */
  std::string start_date_time;
  /**
   * @brief 结束求解时间节点
   * @details 格式 %Y%m%d%H%M%S
   */
  std::string end_date_time;
  /**
   * @brief 总求解时长
   * @details = end_date_time - start_date_time，单位(s)
   */
  long running_time_second{0};
  /**
   * @brief 策略函数耗时列表
   */
  std::vector<VisualFuncTiming> func_timings;

  VisualPlanSummary() = default;
};

namespace VisualJson {
/**
 * @brief 将车次集合序列化为json文本
 * @param visual_loads 车次集合
 * @return json文本, 结构见 resources/template/visual/Loads.json
 */
std::string dumps(const std::vector<VisualLoad>& visual_loads);
/**
 * @brief 将车次站点集合序列化为json文本
 * @param visual_load_locations 车次站点集合
 * @return json文本, 结构见 resources/template/visual/LoadLocations.json
 */
std::string dumps(const std::vector<VisualLoadLocation>& visual_load_locations);
/**
 * @brief 将订单明细集合序列化为json文本
 * @param visual_cargo_orders 订单明细集合
 * @return json文本
 */
std::string dumps(const std::vector<VisualCargoOrder>& visual_cargo_orders);
/**
 * @brief 将不可解订单集合序列化为json文本
 * @param visual_infeasible_cargo_orders 不可解订单集合
 * @return json文本, 字段同 PrecheckInfeasibleCargoOrderSchema::headers
 */
std::string dumps(const std::vector<VisualInfeasibleCargoOrder>& visual_infeasible_cargo_orders);
/**
 * @brief 将规划结果全局汇总序列化为json文本
 * @param summary 规划结果全局汇总
 * @return json文本
 */
std::string dumps(const VisualPlanSummary& summary);
}  // namespace VisualJson
