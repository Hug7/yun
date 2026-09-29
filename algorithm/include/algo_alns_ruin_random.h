/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <sol/sol.hpp>

#include "algo_alns_ruin.h"
#include "c_random_utils.h"

/**
 * @brief ALNS 算法的参数在lua中的字段集合
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

  explicit AlnsRuinRandomLoad(const SolverContext* _context, int default_random_seed);

  explicit AlnsRuinRandomLoad(const SolverContext* _context, int default_random_seed,
                              sol::table cfg);

  void call(Solution* sol) override;
};
