/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#include "algo_alns_parameter.h"

// ====== implement of parse_alns_parameter ======
AlnsParameter* parse_alns_parameter(const sol::table& cfg) {
  const auto alns_parameter = new AlnsParameter();

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
  if (cfg[AlnsParameterLuaField::MAX_RUNNING_TIME_SEC].valid()) {
    alns_parameter->max_running_time_sec =
        static_cast<int>(cfg[AlnsParameterLuaField::MAX_RUNNING_TIME_SEC]);
  }
  if (cfg[AlnsParameterLuaField::OPERATOR_LEARNING_FACTOR].valid()) {
    alns_parameter->operator_learning_factor =
        static_cast<double>(cfg[AlnsParameterLuaField::OPERATOR_LEARNING_FACTOR]);
    if (alns_parameter->operator_learning_factor <= 0 ||
        alns_parameter->operator_learning_factor >= 1.0) {
      throw std::invalid_argument("operator_learning_factor must be > 0 and < 1");
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

  // TODO check 参数

  return alns_parameter;
}
