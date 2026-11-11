/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "algo_alns_ruin.h"
#include "c_random_utils.h"

/**
 * @brief alns ruin算子`RuinRandomRemovalLoad`的参数
 */
struct AlnsRuinRandomRemovalLoadParam {
  /**
   * @brief 随机种子
   */
  int select_load_random_seed{37};
  /**
   * @brief 选择load数的最小比例
   */
  double select_load_min_rate{0.1};
  /**
   * @brief 选择load数的最大比例
   */
  double select_load_max_rate{0.4};
};

/**
 * @brief alns ruin算子随机移除load
 */
class AlnsRuinRandomRemovalLoad : public AlnsRuinOperator {
 public:
  /**
   * @brief 结构化参数
   */
  const AlnsRuinRandomRemovalLoadParam parameter;
  /**
   * @brief 随机数
   */
  RandomUtils::UPtr select_load_random;

  explicit AlnsRuinRandomRemovalLoad(const SolverContext* _context,
                                     const AlnsRuinRandomRemovalLoadParam& _parameter);

  std::unordered_map<int, AlnsRuinLoadActivity::UPtr> execute(Solution* sol) override;

  [[nodiscard]] std::string format_parameters() const override;
};
