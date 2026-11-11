/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#include "stra_alns_config.h"

#include <algorithm>
#include <format>
#include <memory>
#include <stdexcept>
#include <string>
#include <thread>
#include <utility>
#include <vector>

namespace {
/**
 * @brief ALNS 全局参数在 Lua 中的字段集合
 */
class AlnsParameterLuaField {
 public:
  static constexpr const char* WORKER_COUNT = "worker_count";
  static constexpr const char* MAX_ITER = "max_iter";
  static constexpr const char* MAX_SUB_ITER = "max_sub_iter";
  static constexpr const char* MAX_UNIMPROVED_ITER = "max_unimproved_iter";
  static constexpr const char* MAX_STAGNATE_ITER = "max_stagnate_iter";
  static constexpr const char* PRINT_LOG_INTERVAL_ITER = "print_log_interval_iter";
  static constexpr const char* MAX_RUNNING_TIME_SEC = "max_running_time_sec";
  static constexpr const char* OPERATOR_LEARNING_FACTOR = "operator_learning_factor";
  static constexpr const char* OPERATOR_INITIAL_WEIGHT = "operator_initial_weight";
  static constexpr const char* OPERATOR_MIN_WEIGHT = "operator_min_weight";
  static constexpr const char* OPERATOR_WEIGHT_UPDATE_INTERVAL_ITER =
      "operator_weight_update_interval_iter";
  static constexpr const char* PRINT_OPERATOR_STATISTICS = "print_operator_statistics";
  static constexpr const char* INITIAL_TEMPERATURE = "initial_temperature";
  static constexpr const char* MIN_TEMPERATURE = "min_temperature";
  static constexpr const char* ANNEALING_FACTOR = "annealing_factor";
  static constexpr const char* DEFAULT_RANDOM_SEED = "default_random_seed";
};

/**
 * @brief ALNS 算子配置在 Lua 中的字段集合
 */
class AlnsOperatorLuaField {
 public:
  static constexpr const char* RUIN_OPERATOR_PARAMS = "ruin_operator_params";
  static constexpr const char* REPAIR_OPERATOR_PARAMS = "repair_operator_params";
  static constexpr const char* OPERATOR_CODE = "code";
  static constexpr const char* OPERATOR_PARAMS = "params";
};

/**
 * @brief ruin 算子`RuinRandomRemovalLoad`的参数在 Lua 中的字段集合
 */
class AlnsRuinRandomRemovalLoadLuaField {
 public:
  static constexpr const char* SELECT_LOAD_RANDOM_SEED = "select_load_random_seed";
  static constexpr const char* SELECT_LOAD_MIN_RATE = "select_load_min_rate";
  static constexpr const char* SELECT_LOAD_MAX_RATE = "select_load_max_rate";
};

/**
 * @brief ruin 算子`RuinRandomRemovalOrder`的参数在 Lua 中的字段集合
 */
class AlnsRuinRandomRemovalOrderLuaField {
 public:
  static constexpr const char* SELECT_ORDER_RANDOM_SEED = "select_order_random_seed";
  static constexpr const char* SELECT_ORDER_MIN_RATE = "select_order_min_rate";
  static constexpr const char* SELECT_ORDER_MAX_RATE = "select_order_max_rate";
};

/**
 * @brief ruin 算子`RuinWorstRemovalOrder`的参数在 Lua 中的字段集合
 */
class AlnsRuinWorstRemovalOrderLuaField {
 public:
  static constexpr const char* SELECT_ORDER_RANDOM_SEED = "select_order_random_seed";
  static constexpr const char* SELECT_ORDER_MIN_RATE = "select_order_min_rate";
  static constexpr const char* SELECT_ORDER_MAX_RATE = "select_order_max_rate";
  static constexpr const char* SELECT_ORDER_NOISE_EXPONENT = "select_order_noise_exponent";
};

/**
 * @brief repair 算子`RepairGreedy`的参数在 Lua 中的字段集合
 */
class AlnsRepairGreedyLuaField {
 public:
  static constexpr const char* RANDOM_SEED = "random_seed";
  static constexpr const char* TOP_RATE = "top_rate";
};

/**
 * @brief repair 算子`RepairRegretK`的参数在 Lua 中的字段集合
 */
class AlnsRepairRegretKLuaField {
 public:
  static constexpr const char* REGRET_K = "regret_k";
};

// ====== implement of parse_alns_parameter ======
std::unique_ptr<AlnsParameter> parse_alns_parameter(const sol::table& cfg,
                                                    const std::shared_ptr<spdlog::logger>& logger) {
  auto alns_parameter = std::make_unique<AlnsParameter>();

  if (cfg[AlnsParameterLuaField::WORKER_COUNT].valid()) {
    const int worker_count = static_cast<int>(cfg[AlnsParameterLuaField::WORKER_COUNT]);
    if (worker_count == 0 || worker_count < -1) {
      throw std::invalid_argument("AlnsParameter: `worker_count` must be -1 (auto) or > 0");
    }
    const int n = std::max(1, static_cast<int>(std::thread::hardware_concurrency()));
    if (worker_count >= n) {
      const int fallback_worker_count = std::max(1, n - 1);
      logger->warn(
          std::format("AlnsParameter: worker_count={} exceed the maximum number of parallel "
                      "execute {}, degrade the "
                      "number of threads to {}",
                      worker_count, n, fallback_worker_count));
      alns_parameter->worker_count = fallback_worker_count;
    } else {
      alns_parameter->worker_count = worker_count;
    }
  }
  if (cfg[AlnsParameterLuaField::MAX_ITER].valid()) {
    alns_parameter->max_iter = static_cast<int>(cfg[AlnsParameterLuaField::MAX_ITER]);
    if (alns_parameter->max_iter <= 0 || alns_parameter->max_iter > 10000) {
      throw std::invalid_argument("AlnsParameter: `max_iter` must be > 0 and <= 10000");
    }
  }
  if (cfg[AlnsParameterLuaField::MAX_SUB_ITER].valid()) {
    alns_parameter->max_sub_iter = static_cast<int>(cfg[AlnsParameterLuaField::MAX_SUB_ITER]);
    if (alns_parameter->max_sub_iter <= 0 || alns_parameter->max_sub_iter > 40) {
      throw std::invalid_argument("AlnsParameter: `max_sub_iter` must be > 0 and <= 40");
    }
  }
  if (cfg[AlnsParameterLuaField::MAX_UNIMPROVED_ITER].valid()) {
    alns_parameter->max_unimproved_iter =
        static_cast<int>(cfg[AlnsParameterLuaField::MAX_UNIMPROVED_ITER]);
    if (alns_parameter->max_unimproved_iter <= 0) {
      throw std::invalid_argument("AlnsParameter: `max_unimproved_iter` must be > 0");
    }
  }
  if (cfg[AlnsParameterLuaField::MAX_STAGNATE_ITER].valid()) {
    alns_parameter->max_stagnate_iter =
        static_cast<int>(cfg[AlnsParameterLuaField::MAX_STAGNATE_ITER]);
    if (alns_parameter->max_stagnate_iter <= 0) {
      throw std::invalid_argument("AlnsParameter: `max_stagnate_iter` must be > 0");
    }
  }
  if (cfg[AlnsParameterLuaField::PRINT_LOG_INTERVAL_ITER].valid()) {
    alns_parameter->print_log_interval_iter =
        static_cast<int>(cfg[AlnsParameterLuaField::PRINT_LOG_INTERVAL_ITER]);
    if (alns_parameter->print_log_interval_iter <= 0) {
      throw std::invalid_argument("AlnsParameter: `print_log_interval_iter` must be > 0");
    }
  }
  if (cfg[AlnsParameterLuaField::MAX_RUNNING_TIME_SEC].valid()) {
    alns_parameter->max_running_time_sec =
        static_cast<int>(cfg[AlnsParameterLuaField::MAX_RUNNING_TIME_SEC]);
    if (alns_parameter->max_running_time_sec <= 0) {
      throw std::invalid_argument("AlnsParameter: `max_running_time_sec` must be > 0");
    }
  }
  if (cfg[AlnsParameterLuaField::OPERATOR_LEARNING_FACTOR].valid()) {
    alns_parameter->operator_learning_factor =
        static_cast<double>(cfg[AlnsParameterLuaField::OPERATOR_LEARNING_FACTOR]);
    if (alns_parameter->operator_learning_factor <= 0 ||
        alns_parameter->operator_learning_factor >= 1.0) {
      throw std::invalid_argument("AlnsParameter: `operator_learning_factor` must be > 0 and < 1");
    }
  }

  if (cfg[AlnsParameterLuaField::OPERATOR_INITIAL_WEIGHT].valid()) {
    alns_parameter->operator_initial_weight =
        static_cast<double>(cfg[AlnsParameterLuaField::OPERATOR_INITIAL_WEIGHT]);
    if (alns_parameter->operator_initial_weight <= 0) {
      throw std::invalid_argument("AlnsParameter: `operator_initial_weight` must be > 0");
    }
  }

  if (cfg[AlnsParameterLuaField::OPERATOR_MIN_WEIGHT].valid()) {
    alns_parameter->operator_min_weight =
        static_cast<double>(cfg[AlnsParameterLuaField::OPERATOR_MIN_WEIGHT]);
    if (alns_parameter->operator_min_weight <= 0) {
      throw std::invalid_argument("AlnsParameter: `operator_min_weight` must be > 0");
    }
  }
  if (alns_parameter->operator_min_weight >= alns_parameter->operator_initial_weight) {
    throw std::invalid_argument(
        "AlnsParameter: `operator_min_weight` must be < `operator_initial_weight`");
  }

  if (cfg[AlnsParameterLuaField::OPERATOR_WEIGHT_UPDATE_INTERVAL_ITER].valid()) {
    alns_parameter->operator_weight_update_interval_iter =
        static_cast<int>(cfg[AlnsParameterLuaField::OPERATOR_WEIGHT_UPDATE_INTERVAL_ITER]);
    if (alns_parameter->operator_weight_update_interval_iter <= 0 ||
        alns_parameter->operator_weight_update_interval_iter > 20) {
      throw std::invalid_argument(
          "AlnsParameter: `operator_weight_update_interval_iter` must be > 0 and < 20");
    }
  }

  if (cfg[AlnsParameterLuaField::PRINT_OPERATOR_STATISTICS].valid()) {
    const sol::object print_operator_statistics =
        cfg[AlnsParameterLuaField::PRINT_OPERATOR_STATISTICS];
    if (!print_operator_statistics.is<bool>()) {
      throw std::invalid_argument("AlnsParameter: `print_operator_statistics` must be a boolean");
    }
    alns_parameter->print_operator_statistics = print_operator_statistics.as<bool>();
  }

  if (cfg[AlnsParameterLuaField::INITIAL_TEMPERATURE].valid()) {
    alns_parameter->initial_temperature =
        static_cast<double>(cfg[AlnsParameterLuaField::INITIAL_TEMPERATURE]);
  }
  if (cfg[AlnsParameterLuaField::MIN_TEMPERATURE].valid()) {
    alns_parameter->min_temperature =
        static_cast<double>(cfg[AlnsParameterLuaField::MIN_TEMPERATURE]);
  }
  if (cfg[AlnsParameterLuaField::ANNEALING_FACTOR].valid()) {
    alns_parameter->annealing_factor =
        static_cast<double>(cfg[AlnsParameterLuaField::ANNEALING_FACTOR]);
  }
  if (cfg[AlnsParameterLuaField::DEFAULT_RANDOM_SEED].valid()) {
    alns_parameter->default_random_seed =
        static_cast<int>(cfg[AlnsParameterLuaField::DEFAULT_RANDOM_SEED]);
  }

  // metropolis 使用相对温度: P = exp(-delta / (tau * f0)), tau 无量纲
  // 因此要求 0 < tau_end <= tau_0, 且 tau_0 不应大于 1
  if (alns_parameter->initial_temperature <= 0) {
    throw std::invalid_argument("AlnsParameter: `initial_temperature` (tau_0) must be > 0");
  }
  if (alns_parameter->min_temperature <= 0) {
    throw std::invalid_argument("AlnsParameter: `min_temperature` (tau_end) must be > 0");
  }
  if (alns_parameter->min_temperature > alns_parameter->initial_temperature) {
    throw std::invalid_argument(
        "AlnsParameter: `min_temperature` (tau_end) must not exceed `initial_temperature` (tau_0)");
  }
  if (alns_parameter->initial_temperature > 1.0) {
    logger->warn(
        std::format("AlnsParameter: initial_temperature={} looks like an absolute temperature. It "
                    "is now a dimensionless relative temperature tau_0, expected in [1e-3, 1e-1]",
                    alns_parameter->initial_temperature));
  }
  if (alns_parameter->annealing_factor > 0 && alns_parameter->annealing_factor >= 1.0) {
    throw std::invalid_argument(
        "AlnsParameter: `annealing_factor` must be in (0, 1), or <= 0 to derive it automatically");
  }

  logger->info(
      "{}",
      std::format("alns parameters:\n"
                  "  worker_count={}\n"
                  "  max_iter={}\n"
                  "  max_sub_iter={}\n"
                  "  max_unimproved_iter={}\n"
                  "  max_stagnate_iter={}\n"
                  "  print_log_interval_iter={}\n"
                  "  max_running_time_sec={}\n"
                  "  operator_learning_factor={}\n"
                  "  operator_initial_weight={}\n"
                  "  operator_min_weight={}\n"
                  "  operator_weight_update_interval_iter={}\n"
                  "  print_operator_statistics={}\n"
                  "  initial_temperature={}\n"
                  "  min_temperature={}\n"
                  "  annealing_factor={}\n"
                  "  default_random_seed={}",
                  alns_parameter->worker_count, alns_parameter->max_iter,
                  alns_parameter->max_sub_iter, alns_parameter->max_unimproved_iter,
                  alns_parameter->max_stagnate_iter, alns_parameter->print_log_interval_iter,
                  alns_parameter->max_running_time_sec, alns_parameter->operator_learning_factor,
                  alns_parameter->operator_initial_weight, alns_parameter->operator_min_weight,
                  alns_parameter->operator_weight_update_interval_iter,
                  alns_parameter->print_operator_statistics, alns_parameter->initial_temperature,
                  alns_parameter->min_temperature, alns_parameter->annealing_factor,
                  alns_parameter->default_random_seed));

  return alns_parameter;
}

// ====== implement of operator parameter parsing ======
/**
 * @brief 解析 ruin 算子`RuinRandomRemovalLoad`的结构化参数
 */
AlnsRuinRandomRemovalLoadParam parse_ruin_random_removal_load_param(const sol::table& params) {
  AlnsRuinRandomRemovalLoadParam operator_param;
  if (params[AlnsRuinRandomRemovalLoadLuaField::SELECT_LOAD_RANDOM_SEED].valid()) {
    operator_param.select_load_random_seed =
        static_cast<int>(params[AlnsRuinRandomRemovalLoadLuaField::SELECT_LOAD_RANDOM_SEED]);
  }
  if (params[AlnsRuinRandomRemovalLoadLuaField::SELECT_LOAD_MIN_RATE].valid()) {
    operator_param.select_load_min_rate =
        static_cast<double>(params[AlnsRuinRandomRemovalLoadLuaField::SELECT_LOAD_MIN_RATE]);
  }
  if (params[AlnsRuinRandomRemovalLoadLuaField::SELECT_LOAD_MAX_RATE].valid()) {
    operator_param.select_load_max_rate =
        static_cast<double>(params[AlnsRuinRandomRemovalLoadLuaField::SELECT_LOAD_MAX_RATE]);
  }
  return operator_param;
}

/**
 * @brief 解析 ruin 算子`RuinRandomRemovalOrder`的结构化参数
 */
AlnsRuinRandomRemovalOrderParam parse_ruin_random_removal_order_param(const sol::table& params) {
  AlnsRuinRandomRemovalOrderParam operator_param;
  if (params[AlnsRuinRandomRemovalOrderLuaField::SELECT_ORDER_RANDOM_SEED].valid()) {
    operator_param.select_order_random_seed =
        static_cast<int>(params[AlnsRuinRandomRemovalOrderLuaField::SELECT_ORDER_RANDOM_SEED]);
  }
  if (params[AlnsRuinRandomRemovalOrderLuaField::SELECT_ORDER_MIN_RATE].valid()) {
    operator_param.select_order_min_rate =
        static_cast<double>(params[AlnsRuinRandomRemovalOrderLuaField::SELECT_ORDER_MIN_RATE]);
  }
  if (params[AlnsRuinRandomRemovalOrderLuaField::SELECT_ORDER_MAX_RATE].valid()) {
    operator_param.select_order_max_rate =
        static_cast<double>(params[AlnsRuinRandomRemovalOrderLuaField::SELECT_ORDER_MAX_RATE]);
  }
  return operator_param;
}

/**
 * @brief 解析 ruin 算子`RuinWorstRemovalOrder`的结构化参数
 */
AlnsRuinWorstRemovalOrderParam parse_ruin_worst_removal_order_param(const sol::table& params) {
  AlnsRuinWorstRemovalOrderParam operator_param;
  if (params[AlnsRuinWorstRemovalOrderLuaField::SELECT_ORDER_RANDOM_SEED].valid()) {
    operator_param.select_order_random_seed =
        static_cast<int>(params[AlnsRuinWorstRemovalOrderLuaField::SELECT_ORDER_RANDOM_SEED]);
  }
  if (params[AlnsRuinWorstRemovalOrderLuaField::SELECT_ORDER_MIN_RATE].valid()) {
    operator_param.select_order_min_rate =
        static_cast<double>(params[AlnsRuinWorstRemovalOrderLuaField::SELECT_ORDER_MIN_RATE]);
  }
  if (params[AlnsRuinWorstRemovalOrderLuaField::SELECT_ORDER_MAX_RATE].valid()) {
    operator_param.select_order_max_rate =
        static_cast<double>(params[AlnsRuinWorstRemovalOrderLuaField::SELECT_ORDER_MAX_RATE]);
  }
  if (params[AlnsRuinWorstRemovalOrderLuaField::SELECT_ORDER_NOISE_EXPONENT].valid()) {
    operator_param.select_order_noise_exponent =
        static_cast<double>(params[AlnsRuinWorstRemovalOrderLuaField::SELECT_ORDER_NOISE_EXPONENT]);
  }
  return operator_param;
}

/**
 * @brief 解析 repair 算子`RepairGreedy`的结构化参数
 */
AlnsRepairGreedyParam parse_repair_greedy_param(const sol::table& params) {
  AlnsRepairGreedyParam operator_param;
  if (params[AlnsRepairGreedyLuaField::RANDOM_SEED].valid()) {
    operator_param.random_seed = static_cast<int>(params[AlnsRepairGreedyLuaField::RANDOM_SEED]);
  }
  if (params[AlnsRepairGreedyLuaField::TOP_RATE].valid()) {
    operator_param.top_rate = static_cast<double>(params[AlnsRepairGreedyLuaField::TOP_RATE]);
  }
  return operator_param;
}

/**
 * @brief 解析 repair 算子`RepairRegretK`的结构化参数
 */
AlnsRepairRegretKParam parse_repair_regret_k_param(const sol::table& params) {
  AlnsRepairRegretKParam operator_param;
  if (params[AlnsRepairRegretKLuaField::REGRET_K].valid()) {
    operator_param.regret_k = static_cast<int>(params[AlnsRepairRegretKLuaField::REGRET_K]);
  }
  return operator_param;
}

/**
 * @brief 按 code 分发 ruin 算子的结构化参数
 */
AlnsRuinOperatorParam parse_ruin_operator_param(const std::string& code, const sol::table& params) {
  if (code == "RuinRandomRemovalLoad") {
    return parse_ruin_random_removal_load_param(params);
  } else if (code == "RuinRandomRemovalOrder") {
    return parse_ruin_random_removal_order_param(params);
  } else if (code == "RuinWorstRemovalOrder") {
    return parse_ruin_worst_removal_order_param(params);
  }
  throw std::invalid_argument("Invalid AlnsRuinOperatorCode = " + code);
}

/**
 * @brief 按 code 分发 repair 算子的结构化参数
 */
AlnsRepairOperatorParam parse_repair_operator_param(const std::string& code,
                                                    const sol::table& params) {
  if (code == "RepairGreedy") {
    return parse_repair_greedy_param(params);
  } else if (code == "RepairRegretK") {
    return parse_repair_regret_k_param(params);
  }
  throw std::invalid_argument("Invalid AlnsRepairOperatorCode = " + code);
}

/**
 * @brief 取算子的 params 表, 字段缺失时返回空表, 存在但不是表时报错
 */
sol::table get_operator_params(const sol::table& item) {
  const sol::object obj = item[AlnsOperatorLuaField::OPERATOR_PARAMS];
  if (!obj.valid()) {
    return sol::table(item.lua_state(), sol::create);
  }
  if (!obj.is<sol::table>()) {
    throw std::runtime_error(std::string(AlnsOperatorLuaField::OPERATOR_PARAMS) +
                             " must be a table");
  }
  return obj.as<sol::table>();
}

/**
 * @brief 解析一个算子数组
 * @tparam OperatorParam ruin / repair 的结构化参数 variant
 * @param cfg lua 配置表
 * @param array_field 算子数组在 lua 中的字段名
 * @param parse_operator_param 单个算子的 code+params 转换函数
 * @details 数组缺失时返回空; 数组元素不是表时跳过, 与旧实现保持一致
 */
template <typename OperatorParam, typename ParseOperatorParam>
std::vector<OperatorParam> parse_operator_params(const sol::table& cfg, const char* array_field,
                                                 ParseOperatorParam&& parse_operator_param) {
  std::vector<OperatorParam> operator_params;
  if (!cfg[array_field].valid()) {
    return operator_params;
  }
  const sol::table items = cfg[array_field].get<sol::table>();
  items.for_each([&](const sol::object& key, const sol::object& value) {
    // key = 1, 2, 3... (数组下标)
    // value = 每个元素 table
    if (!value.is<sol::table>()) {
      return;  // 跳过非表元素
    }
    const sol::table item = value.as<sol::table>();
    if (!item.valid()) {
      return;  // 防御: 跳过非表元素
    }
    const auto missing_code_error = [&]() {
      return std::runtime_error(std::string(array_field) + "[" + std::to_string(key.as<int>()) +
                                "]: missing '" + AlnsOperatorLuaField::OPERATOR_CODE + "'");
    };
    // 取 code（必填）: 先判类型, 避免 sol2 把 nil 往 string 转换时 panic
    const sol::object code_object = item[AlnsOperatorLuaField::OPERATOR_CODE];
    if (code_object.get_type() != sol::type::string) {
      throw missing_code_error();
    }
    const std::string code = code_object.as<std::string>();
    if (code.empty()) {
      throw missing_code_error();
    }
    // 取 params（可选，缺省给空表）并转换成结构化参数
    operator_params.push_back(parse_operator_param(code, get_operator_params(item)));
  });
  return operator_params;
}
}  // namespace

// ====== implement of parse_alns_strategy_config ======
AlnsStrategyConfig parse_alns_strategy_config(const sol::table& cfg,
                                              const std::shared_ptr<spdlog::logger>& logger) {
  AlnsStrategyConfig config;
  // 全局参数
  config.parameter = parse_alns_parameter(cfg, logger);
  // 算子参数
  config.ruin_operator_params = parse_operator_params<AlnsRuinOperatorParam>(
      cfg, AlnsOperatorLuaField::RUIN_OPERATOR_PARAMS, parse_ruin_operator_param);
  config.repair_operator_params = parse_operator_params<AlnsRepairOperatorParam>(
      cfg, AlnsOperatorLuaField::REPAIR_OPERATOR_PARAMS, parse_repair_operator_param);
  return config;
}
