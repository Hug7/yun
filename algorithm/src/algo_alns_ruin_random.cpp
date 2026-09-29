/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#include "algo_alns_ruin_random.h"

// ====== implement of AlnsRuinRandomLoad ======
AlnsRuinRandomLoad::AlnsRuinRandomLoad(const SolverContext* _context, int default_random_seed)
    : AlnsRuinOperator(AlnsRuinOperatorCode::RuinRandomLoad, _context) {
  this->select_load_random = std::make_unique<RandomUtils>(default_random_seed);
}

AlnsRuinRandomLoad::AlnsRuinRandomLoad(const SolverContext* _context, int default_random_seed,
                                       sol::table cfg)
    : AlnsRuinOperator(AlnsRuinOperatorCode::RuinRandomLoad, _context) {
  if (cfg[AlnsRuinRandomLoadLuaField::SELECT_LOAD_MIN_RATE].valid()) {
    this->select_load_min_rate =
        static_cast<double>(cfg[AlnsRuinRandomLoadLuaField::SELECT_LOAD_MIN_RATE]);
  }
  if (cfg[AlnsRuinRandomLoadLuaField::SELECT_LOAD_MAX_RATE].valid()) {
    this->select_load_max_rate =
        static_cast<double>(cfg[AlnsRuinRandomLoadLuaField::SELECT_LOAD_MAX_RATE]);
  }
  if (cfg[AlnsRuinRandomLoadLuaField::SELECT_LOAD_RANDOM_SEED].valid()) {
    this->select_load_random = std::make_unique<RandomUtils>(
        static_cast<int>(cfg[AlnsRuinRandomLoadLuaField::SELECT_LOAD_RANDOM_SEED]));
  } else {
    this->select_load_random = std::make_unique<RandomUtils>(default_random_seed);
  }

  // 校验参数
  if (this->select_load_min_rate < 0.0 || this->select_load_min_rate > 1.0) {
    throw std::invalid_argument("RuinRandomLoad: select_load_min_rate must be in [0, 1], got " +
                                std::to_string(this->select_load_min_rate));
  }

  if (this->select_load_max_rate < 0.0 || this->select_load_max_rate > 1.0) {
    throw std::runtime_error("RuinRandomLoad: select_load_max_rate must be in [0, 1], got " +
                             std::to_string(this->select_load_max_rate));
  }

  if (this->select_load_min_rate > this->select_load_max_rate) {
    throw std::runtime_error("RuinRandomLoad: select_load_min_rate (" +
                             std::to_string(this->select_load_min_rate) +
                             ") must not exceed select_load_max_rate (" +
                             std::to_string(this->select_load_max_rate) + ")");
  }
}

void AlnsRuinRandomLoad::call(Solution* sol) {
  // TODO: 实现随机破坏逻辑
  // 使用 select_load_min_rate 和 select_load_max_rate 选择载重比例范围
  // 使用 select_load_random 生成随机数
}
