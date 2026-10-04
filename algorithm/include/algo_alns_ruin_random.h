/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <sol/sol.hpp>

#include "algo_alns_ruin.h"
#include "c_random_utils.h"

/**
 * @brief alns ruin算子`RuinRandomLoad`的参数在lua中的字段集合
 */
class AlnsRuinRandomLoadLuaField {
public:
  static constexpr const char* SELECT_LOAD_RANDOM_SEED = "select_load_random_seed";
  static constexpr const char* SELECT_LOAD_MIN_RATE = "select_load_min_rate";
  static constexpr const char* SELECT_LOAD_MAX_RATE = "select_load_max_rate";
};

/**
 * @brief alns ruin算子随机移除load
 */
class AlnsRuinRandomLoad : public AlnsRuinOperator {
 public:
  /**
   * @brief 随机种子
   */
  int select_load_random_seed{37};
  /**
   * @brief 随机数
   */
  RandomUtils::UPtr select_load_random;
  /**
   * @brief 选择load数的最小比例
   */
  double select_load_min_rate{0.1};
  /**
   * @brief 选择load数的最大比例
   */
  double select_load_max_rate{0.4};

  explicit AlnsRuinRandomLoad(const SolverContext* _context, const sol::table& cfg);

  std::unordered_map<int, AlnsRuinLoadActivity::UPtr> execute(Solution* sol) override;

};
