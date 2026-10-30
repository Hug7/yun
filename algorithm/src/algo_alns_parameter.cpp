/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#include "algo_alns_parameter.h"

// ====== implement of parse_alns_parameter ======
std::unique_ptr<AlnsParameter> parse_alns_parameter(const sol::table& cfg,
                                                    const std::shared_ptr<spdlog::logger>& logger) {
  auto alns_parameter = std::make_unique<AlnsParameter>();

  if (cfg[AlnsParameterLuaField::WORKER_COUNT].valid()) {
    const int worker_count = static_cast<int>(cfg[AlnsParameterLuaField::WORKER_COUNT]);
    const int n = static_cast<int>(std::thread::hardware_concurrency());
    if (worker_count >= n) {
      logger->warn(
          std::format("AlnsParameter: worker_count={} exceed the maximum number of parallel "
                      "execute {}, degrade the "
                      "number of threads to {}",
                      worker_count, n, n - 1));
      alns_parameter->worker_count = n - 1;
    } else {
      alns_parameter->worker_count = worker_count;
    }
  }
  if (cfg[AlnsParameterLuaField::MAX_ITER].valid()) {
    alns_parameter->max_iter = static_cast<int>(cfg[AlnsParameterLuaField::MAX_ITER]);
  }
  if (cfg[AlnsParameterLuaField::MAX_SUB_ITER].valid()) {
    alns_parameter->max_sub_iter = static_cast<int>(cfg[AlnsParameterLuaField::MAX_SUB_ITER]);
  }
  if (cfg[AlnsParameterLuaField::MAX_UNIMPROVED_ITER].valid()) {
    alns_parameter->max_unimproved_iter =
        static_cast<int>(cfg[AlnsParameterLuaField::MAX_UNIMPROVED_ITER]);
  }
  if (cfg[AlnsParameterLuaField::PRINT_LOG_INTERVAL_ITER].valid()) {
    alns_parameter->print_log_interval_iter =
        static_cast<int>(cfg[AlnsParameterLuaField::PRINT_LOG_INTERVAL_ITER]);
  }
  if (cfg[AlnsParameterLuaField::MAX_RUNNING_TIME_SEC].valid()) {
    alns_parameter->max_running_time_sec =
        static_cast<int>(cfg[AlnsParameterLuaField::MAX_RUNNING_TIME_SEC]);
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
      throw std::invalid_argument("operator_learning_factor must be >= 1");
    }
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

  // TODO check 参数

  return alns_parameter;
}