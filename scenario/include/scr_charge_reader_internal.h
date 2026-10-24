/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <cmath>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

#include "bdm_charge.h"
#include "se_schema.h"

/**
 * @brief 计费表读取内部工具
 */
namespace ChargeReaderInternal {

inline std::string trim(const std::string& text) {
  const auto begin = text.find_first_not_of(" \t\r\n");
  if (begin == std::string::npos) {
    return "";
  }
  const auto end = text.find_last_not_of(" \t\r\n");
  return text.substr(begin, end - begin + 1);
}

[[noreturn]] inline void fail(const char* file_name, const size_t row, const std::string& message) {
  throw std::runtime_error(std::string(file_name) + " line " + std::to_string(row + 2) + ": " +
                           message);
}

inline std::vector<std::string> split_path_values(const std::string& text) {
  std::vector<std::string> values;
  size_t begin = 0;
  while (begin <= text.size()) {
    const size_t end = text.find('-', begin);
    const std::string token =
        trim(text.substr(begin, end == std::string::npos ? std::string::npos : end - begin));
    if (!token.empty()) {
      values.push_back(token);
    }
    if (end == std::string::npos) {
      break;
    }
    begin = end + 1;
  }
  return values;
}

inline long parse_long(const std::string& text, const char* file_name, const size_t row,
                       const char* field) {
  try {
    size_t pos = 0;
    const long value = std::stol(text, &pos);
    if (pos != text.size()) {
      throw std::invalid_argument("trailing characters");
    }
    return value;
  } catch (const std::exception&) {
    fail(file_name, row, std::string("invalid ") + field + ": " + text);
  }
  return 0;
}

inline double parse_double(const std::string& text, const char* file_name, const size_t row,
                           const char* field) {
  try {
    size_t pos = 0;
    const double value = std::stod(text, &pos);
    if (pos != text.size() || !std::isfinite(value)) {
      throw std::invalid_argument("trailing characters or not finite");
    }
    return value;
  } catch (const std::exception&) {
    fail(file_name, row, std::string("invalid ") + field + ": " + text);
  }
  return 0.0;
}

inline ChargePathType parse_path_type(const std::string& text, const size_t row) {
  if (text.empty()) {
    return ChargePathType::NONE;
  }
  if (text == "OD_PAIR") {
    return ChargePathType::OD_PAIR;
  }
  if (text == "ORDERED_ROUTE") {
    return ChargePathType::ORDERED_ROUTE;
  }
  fail(ChargeRuleSchema::file_name, row, "invalid ChargePathType: " + text);
  return ChargePathType::NONE;
}

inline ChargeConditionType parse_condition_type(const std::string& text, const size_t row) {
  if (text.empty()) {
    return ChargeConditionType::NONE;
  }
  if (text == "DISTANCE") {
    return ChargeConditionType::DISTANCE;
  }
  if (text == "PICK_NODE_COUNT") {
    return ChargeConditionType::PICK_NODE_COUNT;
  }
  if (text == "DROP_NODE_COUNT") {
    return ChargeConditionType::DROP_NODE_COUNT;
  }
  if (text == "DIMENSION") {
    return ChargeConditionType::DIMENSION;
  }
  fail(ChargeRuleSchema::file_name, row, "invalid TierConditionType: " + text);
  return ChargeConditionType::NONE;
}

inline ChargeItemType parse_item_type(const std::string& text, const size_t row) {
  if (text == "FLAT") {
    return ChargeItemType::FLAT;
  }
  if (text == "DISTANCE") {
    return ChargeItemType::DISTANCE;
  }
  if (text == "PICK_STOP") {
    return ChargeItemType::PICK_STOP;
  }
  if (text == "DROP_STOP") {
    return ChargeItemType::DROP_STOP;
  }
  if (text == "DIMENSION") {
    return ChargeItemType::DIMENSION;
  }
  if (text == "DIMENSION_DISTANCE") {
    return ChargeItemType::DIMENSION_DISTANCE;
  }
  fail(ChargeItemSchema::file_name, row, "invalid ItemType: " + text);
  return ChargeItemType::FLAT;
}

inline long power10(const int precision) {
  long scale = 1;
  for (int u = 0; u < precision; u++) {
    scale *= 10;
  }
  return scale;
}

inline long parse_bound(const std::string& text, const ChargeConditionType condition_type,
                        const Dimension* dimension, const bool is_upper, const size_t row) {
  if (condition_type == ChargeConditionType::NONE || text.empty()) {
    return is_upper ? std::numeric_limits<long>::max() : 0;
  }
  switch (condition_type) {
    case ChargeConditionType::DISTANCE: {
      const double km = parse_double(text, ChargeTierSchema::file_name, row, "bound");
      if (km < 0) {
        fail(ChargeTierSchema::file_name, row, "bound must be non-negative");
      }
      return static_cast<long>(
          std::llround(km * static_cast<double>(ChargeParameter::METER_PER_KM)));
    }
    case ChargeConditionType::PICK_NODE_COUNT:
    case ChargeConditionType::DROP_NODE_COUNT: {
      const long value = parse_long(text, ChargeTierSchema::file_name, row, "bound");
      if (value < 0) {
        fail(ChargeTierSchema::file_name, row, "bound must be non-negative");
      }
      return value;
    }
    case ChargeConditionType::DIMENSION: {
      if (dimension == nullptr) {
        fail(ChargeTierSchema::file_name, row, "dimension is required for dimension bound");
      }
      const double value = parse_double(text, ChargeTierSchema::file_name, row, "bound");
      if (value < 0) {
        fail(ChargeTierSchema::file_name, row, "bound must be non-negative");
      }
      const long scale = power10(dimension->precision);
      return static_cast<long>(std::llround(value * static_cast<double>(scale)));
    }
    case ChargeConditionType::NONE:
      break;
  }
  return is_upper ? std::numeric_limits<long>::max() : 0;
}

inline long parse_price(const std::string& text, const size_t row) {
  const double yuan = parse_double(text, ChargeItemSchema::file_name, row, "Price");
  if (yuan < 0) {
    fail(ChargeItemSchema::file_name, row, "Price must be non-negative");
  }
  if (yuan > 1e9) {
    fail(ChargeItemSchema::file_name, row, "Price is too large");
  }
  return static_cast<long>(std::llround(yuan * static_cast<double>(ChargeParameter::MONEY_SCALE)));
}

}  // namespace ChargeReaderInternal
