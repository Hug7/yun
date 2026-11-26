/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#include "stra_ls_config.h"

#include <cmath>
#include <cstdint>
#include <format>
#include <limits>
#include <magic_enum/magic_enum.hpp>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

#include "algo_ls.h"
#include "algo_ls_exchange.h"
#include "algo_ls_operator.h"
#include "algo_ls_segment_relocate.h"
#include "algo_ls_two_opt.h"
#include "algo_ls_two_opt_star.h"

namespace {
/**
 * @brief local search 参数在 Lua 中的字段集合
 */
class LsParameterLuaField {
 public:
  static constexpr const char* EPSILON = "epsilon";
  static constexpr const char* IMPROVEMENT = "improvement";
  static constexpr const char* MAX_EVALUATED = "max_evaluated";
  static constexpr const char* KNN_NEIGHBOR_COUNT = "knn_neighbor_count";
};

/**
 * @brief 算子数组与单算子字段在 Lua 中的字段集合
 */
class LsOperatorLuaField {
 public:
  static constexpr const char* OPERATORS = "operators";
  static constexpr const char* CODE = "code";
  static constexpr const char* PARAMS = "params";
};

/**
 * @brief relocate / or-opt 算子参数在 Lua 中的字段集合
 */
class LsSegmentRelocateLuaField {
 public:
  static constexpr const char* MIN_CHAIN_LEN = "min_chain_len";
  static constexpr const char* MAX_CHAIN_LEN = "max_chain_len";
};

/**
 * @brief 2-opt 算子参数在 Lua 中的字段集合
 */
class LsTwoOptLuaField {
 public:
  static constexpr const char* MIN_SPAN_LEN = "min_span_len";
  static constexpr const char* MAX_SPAN_LEN = "max_span_len";
};

/**
 * @brief 2-opt* 算子参数在 Lua 中的字段集合
 */
class LsTwoOptStarLuaField {
 public:
  static constexpr const char* MIN_TAIL_LEN = "min_tail_len";
  static constexpr const char* MAX_TAIL_LEN = "max_tail_len";
};

/**
 * @brief 拼接错误信息中的字段路径, 形如 `local_search.operators[1].params.min_chain_len`
 */
[[nodiscard]] std::string field_path(const std::string& owner, const char* field) {
  return owner + "." + field;
}

/**
 * @brief 读取可选整型字段
 * @return 字段缺失或为 nil 时返回空; 字段存在但不是数字时抛 `std::invalid_argument`
 */
[[nodiscard]] std::optional<std::int64_t> get_optional_int(const sol::table& cfg, const char* field,
                                                           const std::string& owner) {
  const sol::object value = cfg[field];
  if (!value.valid() || value.get_type() == sol::type::lua_nil) {
    return std::nullopt;
  }
  if (value.get_type() != sol::type::number) {
    throw std::invalid_argument(field_path(owner, field) + " must be a number");
  }
  const double number = value.as<double>();
  if (!std::isfinite(number) ||
      number < static_cast<double>(std::numeric_limits<std::int64_t>::min()) ||
      number > static_cast<double>(std::numeric_limits<std::int64_t>::max())) {
    throw std::invalid_argument(field_path(owner, field) + " must be a finite integer");
  }
  return static_cast<std::int64_t>(number);
}

/**
 * @brief 读取可选浮点字段
 * @return 字段缺失或为 nil 时返回空; 字段存在但不是数字时抛 `std::invalid_argument`
 */
[[nodiscard]] std::optional<double> get_optional_double(const sol::table& cfg, const char* field,
                                                        const std::string& owner) {
  const sol::object value = cfg[field];
  if (!value.valid() || value.get_type() == sol::type::lua_nil) {
    return std::nullopt;
  }
  if (value.get_type() != sol::type::number) {
    throw std::invalid_argument(field_path(owner, field) + " must be a number");
  }
  const double number = value.as<double>();
  if (!std::isfinite(number)) {
    throw std::invalid_argument(field_path(owner, field) + " must be finite");
  }
  return number;
}

/**
 * @brief 读取可选字符串字段
 * @return 字段缺失或为 nil 时返回空; 字段存在但不是字符串时抛 `std::invalid_argument`
 */
[[nodiscard]] std::optional<std::string> get_optional_string(const sol::table& cfg,
                                                             const char* field,
                                                             const std::string& owner) {
  const sol::object value = cfg[field];
  if (!value.valid() || value.get_type() == sol::type::lua_nil) {
    return std::nullopt;
  }
  if (value.get_type() != sol::type::string) {
    throw std::invalid_argument(field_path(owner, field) + " must be a string");
  }
  return value.as<std::string>();
}

/**
 * @brief 把 Lua 里的改进策略字符串转成枚举
 */
[[nodiscard]] LsImprovement parse_improvement(const std::string& name) {
  if (name == "first") {
    return LsImprovement::First;
  }
  if (name == "best") {
    return LsImprovement::Best;
  }
  throw std::invalid_argument("local_search: invalid improvement `improvement` = " + name);
}

/**
 * @brief 解析 local search 的标量参数
 * @param cfg local search 配置表
 * @param defaults 未显式配置字段的默认值
 */
[[nodiscard]] LsParameter parse_ls_parameter(const sol::table& cfg, const LsParameter& defaults) {
  const std::string owner = "local_search";
  LsParameter parameter = defaults;
  if (const auto value = get_optional_double(cfg, LsParameterLuaField::EPSILON, owner)) {
    parameter.epsilon = *value;
  }
  if (const auto value = get_optional_string(cfg, LsParameterLuaField::IMPROVEMENT, owner)) {
    parameter.improvement = parse_improvement(*value);
  }
  if (const auto value = get_optional_int(cfg, LsParameterLuaField::MAX_EVALUATED, owner)) {
    if (*value < 0 || *value > static_cast<std::int64_t>(std::numeric_limits<int>::max())) {
      throw std::invalid_argument(field_path(owner, LsParameterLuaField::MAX_EVALUATED) +
                                  " must be within [0, INT_MAX]");
    }
    parameter.max_evaluated = static_cast<int>(*value);
  }
  if (const auto value = get_optional_int(cfg, LsParameterLuaField::KNN_NEIGHBOR_COUNT, owner)) {
    if (*value < 0 || *value > static_cast<std::int64_t>(std::numeric_limits<int>::max())) {
      throw std::invalid_argument(field_path(owner, LsParameterLuaField::KNN_NEIGHBOR_COUNT) +
                                  " must be within [0, INT_MAX]");
    }
    parameter.knn_neighbor_count = static_cast<int>(*value);
  }
  return parameter;
}

/**
 * @brief 取算子的 params 表, 字段缺失时返回空表, 存在但不是表时报错
 */
[[nodiscard]] sol::table get_operator_params(const sol::table& item, const std::string& owner) {
  const sol::object object = item[LsOperatorLuaField::PARAMS];
  if (!object.valid() || object.get_type() == sol::type::lua_nil) {
    return sol::table(item.lua_state(), sol::create);
  }
  if (object.get_type() != sol::type::table) {
    throw std::invalid_argument(field_path(owner, LsOperatorLuaField::PARAMS) + " must be a table");
  }
  return object.as<sol::table>();
}

/**
 * @brief 解析 relocate / or-opt 算子的结构化参数
 */
[[nodiscard]] LsSegmentRelocateParam parse_ls_segment_relocate_param(const sol::table& params,
                                                                     const std::string& owner) {
  LsSegmentRelocateParam operator_param;
  if (const auto value =
          get_optional_int(params, LsSegmentRelocateLuaField::MIN_CHAIN_LEN, owner)) {
    operator_param.min_chain_len = static_cast<int>(*value);
  }
  if (const auto value =
          get_optional_int(params, LsSegmentRelocateLuaField::MAX_CHAIN_LEN, owner)) {
    operator_param.max_chain_len = static_cast<int>(*value);
  }
  return operator_param;
}

/**
 * @brief 解析 2-opt 算子的结构化参数
 */
[[nodiscard]] LsTwoOptParam parse_ls_two_opt_param(const sol::table& params,
                                                   const std::string& owner) {
  LsTwoOptParam operator_param;
  if (const auto value = get_optional_int(params, LsTwoOptLuaField::MIN_SPAN_LEN, owner)) {
    operator_param.min_span_len = static_cast<int>(*value);
  }
  if (const auto value = get_optional_int(params, LsTwoOptLuaField::MAX_SPAN_LEN, owner)) {
    operator_param.max_span_len = static_cast<int>(*value);
  }
  return operator_param;
}

/**
 * @brief 解析 2-opt* 算子的结构化参数
 */
[[nodiscard]] LsTwoOptStarParam parse_ls_two_opt_star_param(const sol::table& params,
                                                            const std::string& owner) {
  LsTwoOptStarParam operator_param;
  if (const auto value = get_optional_int(params, LsTwoOptStarLuaField::MIN_TAIL_LEN, owner)) {
    operator_param.min_tail_len = static_cast<int>(*value);
  }
  if (const auto value = get_optional_int(params, LsTwoOptStarLuaField::MAX_TAIL_LEN, owner)) {
    operator_param.max_tail_len = static_cast<int>(*value);
  }
  return operator_param;
}

/**
 * @brief 按 code 分发算子的结构化参数, code 用 magic_enum 名称匹配
 */
[[nodiscard]] LsOperatorParam parse_ls_operator_param(const std::string& code,
                                                      const sol::table& params,
                                                      const std::string& owner) {
  const auto operator_code = magic_enum::enum_cast<LsOperatorCode>(code);
  if (!operator_code.has_value()) {
    throw std::invalid_argument("local_search: invalid operator `code` = " + code);
  }
  switch (*operator_code) {
    case LsOperatorCode::LsIntraRouteRelocate: {
      LsSegmentRelocateParam operator_param = parse_ls_segment_relocate_param(params, owner);
      operator_param.route_scope = LsRouteScope::Intra;
      return operator_param;
    }
    case LsOperatorCode::LsInterRouteRelocate: {
      LsSegmentRelocateParam operator_param = parse_ls_segment_relocate_param(params, owner);
      operator_param.route_scope = LsRouteScope::Inter;
      return operator_param;
    }
    case LsOperatorCode::LsIntraRouteExchange:
      return LsExchangeParam{.route_scope = LsRouteScope::Intra};
    case LsOperatorCode::LsInterRouteExchange:
      return LsExchangeParam{.route_scope = LsRouteScope::Inter};
    case LsOperatorCode::LsTwoOpt:
      return parse_ls_two_opt_param(params, owner);
    case LsOperatorCode::LsTwoOptStar:
      return parse_ls_two_opt_star_param(params, owner);
  }
  throw std::invalid_argument("local_search: invalid operator `code` = " + code);
}

/**
 * @brief 解析算子实例数组; 数组缺失时返回空, 由算法层展开为默认 7 邻域
 */
[[nodiscard]] std::vector<LsOperatorParam> parse_ls_operator_params(
    const sol::object& operators_object) {
  std::vector<LsOperatorParam> operator_params;
  if (!operators_object.valid() || operators_object.get_type() == sol::type::lua_nil) {
    return operator_params;
  }
  if (operators_object.get_type() != sol::type::table) {
    throw std::invalid_argument("local_search: `operators` must be a table");
  }
  const sol::table operators = operators_object.as<sol::table>();
  const std::size_t operator_count = operators.size();
  std::size_t pair_count = 0;
  operators.for_each(
      [&pair_count](const sol::object& /*key*/, const sol::object& /*value*/) { ++pair_count; });
  if (pair_count != operator_count) {
    throw std::invalid_argument("local_search: `operators` must be a sequence array");
  }
  // 按 1..n 顺序读取，避免 Lua 表遍历顺序影响算子实例顺序
  operator_params.reserve(operator_count);
  for (std::size_t pos = 0; pos < operator_count; ++pos) {
    const int ind = static_cast<int>(pos) + 1;
    const std::string owner = std::format("local_search.operators[{}]", ind);
    const sol::object value = operators[ind];
    if (value.get_type() != sol::type::table) {
      throw std::invalid_argument(owner + " must be a table");
    }
    const sol::table item = value.as<sol::table>();
    const sol::object code_object = item[LsOperatorLuaField::CODE];
    if (code_object.get_type() != sol::type::string) {
      throw std::invalid_argument(owner + " missing `code`");
    }
    const std::string code = code_object.as<std::string>();
    operator_params.push_back(
        parse_ls_operator_param(code, get_operator_params(item, owner), owner));
  }
  return operator_params;
}
}  // namespace

// ====== implement of parse_local_search_strategy_config ======
LsStrategyConfig parse_local_search_strategy_config(const sol::object& cfg,
                                                    const LsParameter& defaults,
                                                    const std::shared_ptr<spdlog::logger>& logger) {
  LsStrategyConfig config;
  config.parameter = defaults;
  // 不传参数或传入 nil 时使用默认值
  if (!cfg.valid() || cfg.get_type() == sol::type::lua_nil) {
    return config;
  }
  if (cfg.get_type() != sol::type::table) {
    throw std::invalid_argument("local_search: parameter must be a table or nil");
  }
  const sol::table table = cfg.as<sol::table>();
  config.parameter = parse_ls_parameter(table, defaults);
  config.operators = parse_ls_operator_params(table[LsOperatorLuaField::OPERATORS]);
  if (logger != nullptr) {
    logger->info(
        "local_search config: improvement={}, max_evaluated={}, knn_neighbor_count={}, "
        "operator_count={}",
        config.parameter.improvement == LsImprovement::First ? "first" : "best",
        config.parameter.max_evaluated, config.parameter.knn_neighbor_count,
        config.operators.size());
  }
  return config;
}
