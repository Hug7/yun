/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#include "visual_load.h"

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
    if (ch == '"') {
      result += "\\\"";
    } else if (ch == '\\') {
      result += "\\\\";
    } else if (ch < 0x20) {
      result += std::format("\\u{:04x}", static_cast<int>(ch));
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
 * @brief 单个车次转为json对象
 * @param visual_load 车次
 * @param indent 对象自身的缩进
 */
std::string load_to_json(const VisualLoad& visual_load, const std::string& indent) {
  const std::string pad = indent + "  ";
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
  std::string result = indent + "{\n";
  for (size_t i = 0; i < fields.size(); ++i) {
    result += pad + fields[i] + (i + 1 < fields.size() ? ",\n" : "\n");
  }
  return result + indent + "}";
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
