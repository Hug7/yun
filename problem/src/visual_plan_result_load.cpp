/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#include "visual_plan_result_load.h"

#include <cmath>
#include <format>
#include <string>
#include <vector>

namespace {
/**
 * @brief 转义json字符串中的特殊字符
 */
std::string escape(const std::string& raw) {
  std::string result;
  result.reserve(raw.size());
  for (const char ch : raw) {
    // char 在本平台是有符号的, 直接比较会把utf-8多字节(>=0x80)误判为控制字符
    const auto code = static_cast<unsigned int>(static_cast<unsigned char>(ch));
    if (ch == '"') {
      result += "\\\"";
    } else if (ch == '\\') {
      result += "\\\\";
    } else if (code < 0x20) {
      result += std::format("\\u{:04x}", code);
    } else {
      result += ch;
    }
  }
  return result;
}

/**
 * @brief 转为json字符串字面量
 */
std::string str(const std::string& value) { return std::format("\"{}\"", escape(value)); }

/**
 * @brief 转为json数值字面量
 * @details 非有限值(nan、inf)会让json失效, 一律记为0
 */
std::string num(const double value) {
  return std::isfinite(value) ? std::format("{}", value) : "0";
}

/**
 * @brief 维度集合转为单行json数组
 */
std::string dims_to_json(const std::vector<VisualDimension>& dimensions) {
  std::string result = "[";
  for (size_t i = 0; i < dimensions.size(); ++i) {
    if (i > 0) {
      result += ", ";
    }
    result += std::format("{{\"DimensionCode\": {}, \"DimensionValue\": {}}}",
                          str(dimensions[i].dimension_code), num(dimensions[i].dimension_value));
  }
  return result + "]";
}

/**
 * @brief 标签集合转为单行json数组
 */
std::string labels_to_json(const std::vector<VisualLabel>& labels) {
  std::string result = "[";
  for (size_t i = 0; i < labels.size(); ++i) {
    if (i > 0) {
      result += ", ";
    }
    std::string values = "[";
    for (size_t v = 0; v < labels[i].label_values.size(); ++v) {
      if (v > 0) {
        values += ", ";
      }
      values += str(labels[i].label_values[v]);
    }
    result += std::format("{{\"LabelCode\": {}, \"LabelValues\": {}}}", str(labels[i].label_code),
                          values + "]");
  }
  return result + "]";
}

/**
 * @brief 字符串集合转为单行json数组
 */
std::string strs_to_json(const std::vector<std::string>& values) {
  std::string result = "[";
  for (size_t i = 0; i < values.size(); ++i) {
    if (i > 0) {
      result += ", ";
    }
    result += str(values[i]);
  }
  return result + "]";
}

/**
 * @brief 字段集合拼装为多行json对象
 * @param fields "key": value 形式的字段集合
 * @param indent 对象自身的缩进
 */
std::string obj_to_json(const std::vector<std::string>& fields, const std::string& indent) {
  const std::string pad = indent + "  ";
  std::string result = indent + "{\n";
  for (size_t i = 0; i < fields.size(); ++i) {
    result += pad + fields[i] + (i + 1 < fields.size() ? ",\n" : "\n");
  }
  return result + indent + "}";
}

/**
 * @brief 已渲染的json对象集合拼装为多行数组
 * @param objects 对象json文本集合, 每个元素自带缩进
 * @param indent 数组收尾的缩进
 */
std::string arr_to_json(const std::vector<std::string>& objects, const std::string& indent) {
  if (objects.empty()) {
    return "[]";
  }
  std::string result = "[\n";
  for (size_t i = 0; i < objects.size(); ++i) {
    result += objects[i] + (i + 1 < objects.size() ? ",\n" : "\n");
  }
  return result + indent + "]";
}

/**
 * @brief 单个车次转为json对象
 * @param visual_load 车次
 * @param indent 对象自身的缩进
 */
std::string load_to_json(const VisualLoad& visual_load, const std::string& indent) {
  const std::vector<std::string> fields = {
      std::format("\"LoadCode\": {}", str(visual_load.load_code)),
      std::format("\"CarrierCode\": {}", str(visual_load.carrier_code)),
      std::format("\"VehicleModelCode\": {}", str(visual_load.vehicle_model_code)),
      std::format("\"VehicleLoadDimensions\": {}",
                  dims_to_json(visual_load.vehicle_load_dimensions)),
      std::format("\"LoadDimensions\": {}", dims_to_json(visual_load.load_dimensions)),
      std::format("\"LoadingRateDimensions\": {}",
                  dims_to_json(visual_load.loading_rate_dimensions)),
      std::format("\"LoadingDimensionCode\": {}", str(visual_load.loading_dimension_code)),
      std::format("\"LoadingRate\": {}", num(visual_load.loading_rate)),
      std::format("\"TotalDrivingDistanceMeter\": {}", visual_load.total_driving_distance_meter),
      std::format("\"TotalDrivingTimeSecond\": {}", visual_load.total_driving_time_second),
      std::format("\"TotalWorkTimeSecond\": {}", visual_load.total_work_time_second),
      std::format("\"TotalWaitTimeSecond\": {}", visual_load.total_wait_time_second),
      std::format("\"TotalCostTimeSecond\": {}", visual_load.total_cost_time_second),
      std::format("\"SoftPenaltyValue\": {}", num(visual_load.soft_penalty_value)),
      std::format("\"CostValue\": {}", num(visual_load.cost_value)),
      std::format("\"ObjectValue\": {}", num(visual_load.object_value)),
      std::format("\"PickLocationLabels\": {}", labels_to_json(visual_load.pick_location_labels)),
      std::format("\"DropLocationLabels\": {}", labels_to_json(visual_load.drop_location_labels)),
      std::format("\"OrderLabels\": {}", labels_to_json(visual_load.order_labels)),
      std::format("\"LocationSequences\": {}", strs_to_json(visual_load.location_sequences)),
      std::format("\"PlanEarliestStartTime\": {}", str(visual_load.plan_earliest_start_time)),
      std::format("\"PlanEarliestStartTimestamp\": {}", visual_load.plan_earliest_start_timestamp),
      std::format("\"PlanEarliestEndTime\": {}", str(visual_load.plan_earliest_end_time)),
      std::format("\"PlanEarliestEndTimestamp\": {}", visual_load.plan_earliest_end_timestamp),
      std::format("\"PlanLatestStartTime\": {}", str(visual_load.plan_latest_start_time)),
      std::format("\"PlanLatestStartTimestamp\": {}", visual_load.plan_latest_start_timestamp),
      std::format("\"PlanLatestEndTime\": {}", str(visual_load.plan_latest_end_time)),
      std::format("\"PlanLatestEndTimestamp\": {}", visual_load.plan_latest_end_timestamp),
  };
  return obj_to_json(fields, indent);
}

/**
 * @brief 单个订单明细转为json对象
 * @param cargo_order 订单明细
 * @param indent 对象自身的缩进
 */
std::string cargo_order_to_json(const VisualCargoOrder& cargo_order, const std::string& indent) {
  const std::vector<std::string> fields = {
      std::format("\"CargoOrderCode\": {}", str(cargo_order.cargo_order_code)),
      std::format("\"CargoSubOrderCode\": {}", str(cargo_order.cargo_sub_order_code)),
      std::format("\"Quantity\": {}", cargo_order.quantity),
      std::format("\"OrderDimensions\": {}", dims_to_json(cargo_order.order_dimensions)),
      std::format("\"PickLocationCode\": {}", str(cargo_order.pick_location_code)),
      std::format("\"DropLocationCode\": {}", str(cargo_order.drop_location_code)),
      std::format("\"OrderLabels\": {}", labels_to_json(cargo_order.order_labels)),
  };
  return obj_to_json(fields, indent);
}

/**
 * @brief 单个车次站点转为json对象
 * @param load_location 车次站点
 * @param indent 对象自身的缩进
 */
std::string load_location_to_json(const VisualLoadLocation& load_location,
                                  const std::string& indent) {
  const std::string pad = indent + "  ";
  std::vector<std::string> cargo_orders;
  cargo_orders.reserve(load_location.cargo_orders.size());
  for (const auto& cargo_order : load_location.cargo_orders) {
    cargo_orders.emplace_back(cargo_order_to_json(cargo_order, pad + "  "));
  }
  const std::vector<std::string> fields = {
      std::format("\"LoadCode\": {}", str(load_location.load_code)),
      std::format("\"CarrierCode\": {}", str(load_location.carrier_code)),
      std::format("\"VehicleModelCode\": {}", str(load_location.vehicle_model_code)),
      std::format("\"LocationCode\": {}", str(load_location.location_code)),
      std::format("\"ActivityType\": {}", str(load_location.activity_type)),
      std::format("\"CargoOrders\": {}", arr_to_json(cargo_orders, pad)),
      std::format("\"LocationLabels\": {}", labels_to_json(load_location.location_labels)),
      std::format("\"OrderLabels\": {}", labels_to_json(load_location.order_labels)),
      std::format("\"PrevDrivingDistanceMeter\": {}", load_location.prev_driving_distance_meter),
      std::format("\"PrevDrivingTimeSecond\": {}", load_location.prev_driving_time_second),
      std::format("\"WaitTimeSecond\": {}", load_location.wait_time_second),
      std::format("\"WorkTimeSecond\": {}", load_location.work_time_second),
      std::format("\"PlanEarliestArriveTime\": {}", str(load_location.plan_earliest_arrive_time)),
      std::format("\"PlanEarliestArriveTimestamp\": {}",
                  load_location.plan_earliest_arrive_timestamp),
      std::format("\"PlanEarliestStartWorkTime\": {}",
                  str(load_location.plan_earliest_start_work_time)),
      std::format("\"PlanEarliestStartWorkTimestamp\": {}",
                  load_location.plan_earliest_start_work_timestamp),
      std::format("\"PlanEarliestDepartTime\": {}", str(load_location.plan_earliest_depart_time)),
      std::format("\"PlanEarliestDepartTimestamp\": {}",
                  load_location.plan_earliest_depart_timestamp),
      std::format("\"PlanLatestArriveTime\": {}", str(load_location.plan_latest_arrive_time)),
      std::format("\"PlanLatestArriveTimestamp\": {}", load_location.plan_latest_arrive_timestamp),
      std::format("\"PlanLatestStartWorkTime\": {}",
                  str(load_location.plan_latest_start_work_time)),
      std::format("\"PlanLatestStartWorkTimestamp\": {}",
                  load_location.plan_latest_start_work_timestamp),
      std::format("\"PlanLatestDepartTime\": {}", str(load_location.plan_latest_depart_time)),
      std::format("\"PlanLatestDepartTimestamp\": {}", load_location.plan_latest_depart_timestamp),
  };
  return obj_to_json(fields, indent);
}

/**
 * @brief 单个不可解订单记录转为json对象
 * @param infeasible_cargo_order 不可解订单记录
 * @param indent 对象自身的缩进
 */
std::string infeasible_cargo_order_to_json(const VisualInfeasibleCargoOrder& infeasible_cargo_order,
                                           const std::string& indent) {
  const std::vector<std::string> fields = {
      std::format("\"CargoOrderCode\": {}", str(infeasible_cargo_order.cargo_order_code)),
      std::format("\"CarrierCode\": {}", str(infeasible_cargo_order.carrier_code)),
      std::format("\"VehicleModelCode\": {}", str(infeasible_cargo_order.vehicle_model_code)),
      std::format("\"InfeasibleReasonCode\": {}",
                  str(infeasible_cargo_order.infeasible_reason_code)),
      std::format("\"InfeasibleReasonConstraintCode\": {}",
                  str(infeasible_cargo_order.infeasible_reason_constr_code)),
      std::format("\"InfeasibleReasonMessageCN\": {}",
                  str(infeasible_cargo_order.infeasible_reason_message_cn)),
      std::format("\"InfeasibleReasonMessageEN\": {}",
                  str(infeasible_cargo_order.infeasible_reason_message_en)),
  };
  return obj_to_json(fields, indent);
}
}  // namespace

std::string VisualJson::dumps(const std::vector<VisualLoad>& visual_loads) {
  if (visual_loads.empty()) {
    return "[]\n";
  }
  std::string result = "[\n";
  for (size_t i = 0; i < visual_loads.size(); ++i) {
    result += load_to_json(visual_loads[i], "  ") + (i + 1 < visual_loads.size() ? ",\n" : "\n");
  }
  return result + "]\n";
}

std::string VisualJson::dumps(const std::vector<VisualLoadLocation>& visual_load_locations) {
  if (visual_load_locations.empty()) {
    return "[]\n";
  }
  std::string result = "[\n";
  for (size_t i = 0; i < visual_load_locations.size(); ++i) {
    result += load_location_to_json(visual_load_locations[i], "  ") +
              (i + 1 < visual_load_locations.size() ? ",\n" : "\n");
  }
  return result + "]\n";
}

std::string VisualJson::dumps(const std::vector<VisualCargoOrder>& visual_cargo_orders) {
  if (visual_cargo_orders.empty()) {
    return "[]\n";
  }
  std::string result = "[\n";
  for (size_t i = 0; i < visual_cargo_orders.size(); ++i) {
    result += cargo_order_to_json(visual_cargo_orders[i], "  ") +
              (i + 1 < visual_cargo_orders.size() ? ",\n" : "\n");
  }
  return result + "]\n";
}

std::string VisualJson::dumps(
    const std::vector<VisualInfeasibleCargoOrder>& visual_infeasible_cargo_orders) {
  if (visual_infeasible_cargo_orders.empty()) {
    return "[]\n";
  }
  std::string result = "[\n";
  for (size_t i = 0; i < visual_infeasible_cargo_orders.size(); ++i) {
    result += infeasible_cargo_order_to_json(visual_infeasible_cargo_orders[i], "  ") +
              (i + 1 < visual_infeasible_cargo_orders.size() ? ",\n" : "\n");
  }
  return result + "]\n";
}
