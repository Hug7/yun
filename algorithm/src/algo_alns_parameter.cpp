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
    alns_parameter->max_unimproved_iter = static_cast<int>(cfg[AlnsParameterLuaField::MAX_UNIMPROVED_ITER]);
  }
  if (cfg[AlnsParameterLuaField::MAX_RUNNING_TIME_SEC].valid()) {
    alns_parameter->max_running_time_sec = static_cast<int>(cfg[AlnsParameterLuaField::MAX_RUNNING_TIME_SEC]);
  }
  if (cfg[AlnsParameterLuaField::ANNEALING_FACTOR].valid()) {
    alns_parameter->annealing_factor = static_cast<double>(cfg[AlnsParameterLuaField::ANNEALING_FACTOR]);
  }
  if (cfg[AlnsParameterLuaField::DEFAULT_RANDOM_SEED].valid()) {
    alns_parameter->default_random_seed = static_cast<int>(cfg[AlnsParameterLuaField::DEFAULT_RANDOM_SEED]);
  }

  // TODO check 参数

  return alns_parameter;
}
