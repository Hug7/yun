/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#include "algo_alns_parameter.h"

#include <algorithm>
#include <format>
#include <stdexcept>
#include <thread>

// ====== implement of parse_alns_parameter ======
std::unique_ptr<AlnsParameter> parse_alns_parameter(const sol::table& cfg,
                                                    const std::shared_ptr<spdlog::logger>& logger) {
  auto alns_parameter = std::make_unique<AlnsParameter>();

  if (cfg[AlnsParameterLuaField::WORKER_COUNT].valid()) {
    const int worker_count = static_cast<int>(cfg[AlnsParameterLuaField::WORKER_COUNT]);
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
    if (alns_parameter->operator_initial_weight < 1) {
      throw std::invalid_argument("AlnsParameter: `operator_initial_weight` must be >= 1");
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
                  alns_parameter->print_operator_statistics, alns_parameter->initial_temperature,
                  alns_parameter->min_temperature, alns_parameter->annealing_factor,
                  alns_parameter->default_random_seed));

  return alns_parameter;
}