/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#include "algo_alns_ruin_random.h"

// ====== implement of AlnsRuinRandomLoad ======
AlnsRuinRandomLoad::AlnsRuinRandomLoad(const SolverContext* _context, const sol::table& cfg)
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
    this->select_load_random_seed =
        static_cast<int>(cfg[AlnsRuinRandomLoadLuaField::SELECT_LOAD_RANDOM_SEED]);
  }
  this->select_load_random = std::make_unique<RandomUtils>(this->select_load_random_seed);

  // 校验参数
  if (this->select_load_min_rate < 0.0 || this->select_load_min_rate > 1.0) {
    throw std::invalid_argument(this->code + ": `select_load_min_rate` must be in [0, 1], got " +
                                std::to_string(this->select_load_min_rate));
  }

  if (this->select_load_max_rate < 0.0 || this->select_load_max_rate > 1.0) {
    throw std::runtime_error(this->code + ": `select_load_max_rate` must be in [0, 1], got " +
                             std::to_string(this->select_load_max_rate));
  }

  if (this->select_load_min_rate > this->select_load_max_rate) {
    throw std::runtime_error(this->code + ": `select_load_min_rate` (" +
                             std::to_string(this->select_load_min_rate) +
                             ") must not exceed select_load_max_rate (" +
                             std::to_string(this->select_load_max_rate) + ")");
  }
}

std::unordered_map<int, AlnsRuinLoadActivity::UPtr> AlnsRuinRandomLoad::execute(Solution* sol) {
  // 当前解空间
  const int load_count = static_cast<int>(sol->loads.size());
  // ruin的load数量
  const int select_load_count =
      std::max(1, this->select_load_random->random_int(load_count, this->select_load_min_rate,
                                                       this->select_load_max_rate));
  // load索引的随机排序
  auto random_load_indices = this->select_load_random->shuffled_range(load_count);
  // 选中指定数量的load
  std::unordered_map<int, AlnsRuinLoadActivity::UPtr> select_load_res;
  int cur_select_load_count = 0;
  for (int u = 0; u < load_count; ++u) {
    if (cur_select_load_count >= select_load_count) {
      break;
    }
    ++cur_select_load_count;
    select_load_res[u] = std::make_unique<AlnsRuinLoadActivity>(random_load_indices[u]);
  }

  return select_load_res;
}
