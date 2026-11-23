/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "algo_alns_ruin.h"
#include "c_random_utils.h"

/**
 * @brief alns ruin算子`RuinRandomRemovalOrder`的参数
 */
struct AlnsRuinRandomRemovalOrderParam {
  /**
   * @brief 随机种子
   */
  int select_order_random_seed{37};
  /**
   * @brief 选择order数的最小比例
   */
  double select_order_min_rate{0.1};
  /**
   * @brief 选择order数的最大比例
   */
  double select_order_max_rate{0.4};
};

/**
 * @brief alns ruin算子随机移除order
 */
class AlnsRuinRandomRemovalOrder : public AlnsRuinOperator {
 public:
  /**
   * @brief 结构化参数
   */
  const AlnsRuinRandomRemovalOrderParam parameter;
  /**
   * @brief 随机数
   */
  RandomUtils::UPtr select_order_random;

  explicit AlnsRuinRandomRemovalOrder(const SolverContext* _context,
                                      const AlnsRuinRandomRemovalOrderParam& _parameter);

  std::unordered_map<int, AlnsRuinLoadActivity::UPtr> execute(Solution* sol) override;

  [[nodiscard]] std::string format_parameters() const override;
};
