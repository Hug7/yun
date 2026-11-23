/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#include "stra_construct_config.h"

#include <stdexcept>

namespace {
/**
 * @brief `construct_knn` 参数在 Lua 中的字段集合
 */
class ConstructKnnLuaField {
 public:
  static constexpr const char* NEIGHBOR_COUNT = "neighbor_count";
};
}  // namespace

// ====== implement of parse_construct_knn_parameter ======
KnnParameter parse_construct_knn_parameter(const sol::object& parameter) {
  // 不传参数或传入 nil 时使用结构体默认值
  if (!parameter.valid() || parameter.get_type() == sol::type::nil) {
    return KnnParameter{};
  }
  if (parameter.get_type() != sol::type::table) {
    throw std::runtime_error("construct_knn: parameter must be a table");
  }

  KnnParameter knn_parameter;
  const sol::table cfg = parameter.as<sol::table>();
  if (cfg[ConstructKnnLuaField::NEIGHBOR_COUNT].valid()) {
    knn_parameter.neighbor_count = static_cast<int>(cfg[ConstructKnnLuaField::NEIGHBOR_COUNT]);
  }
  return knn_parameter;
}
