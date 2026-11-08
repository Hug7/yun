/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <cstddef>
#include <sol/sol.hpp>
#include <vector>

#include "algo_alns_ruin.h"
#include "c_random_utils.h"

/**
 * @brief alns ruin算子`RuinWorstRemovalOrder`的参数在lua中的字段集合
 */
class AlnsRuinWorstRemovalOrderLuaField {
 public:
  static constexpr const char* SELECT_ORDER_RANDOM_SEED = "select_order_random_seed";
  static constexpr const char* SELECT_ORDER_MIN_RATE = "select_order_min_rate";
  static constexpr const char* SELECT_ORDER_MAX_RATE = "select_order_max_rate";
  static constexpr const char* SELECT_ORDER_NOISE_EXPONENT = "select_order_noise_exponent";
};

/**
 * @brief 从收益数组中挑选 `count` 个收益最大的候选
 * @details 先把下标按收益降序排序(收益相同时按下标升序, 保证可复现), 然后第 u 次取
 * `pos = floor(|pool| * y^p)`, `y~U(0,1)`, 取出该位置的下标并从池中弹出。
 * `p` 越大越集中在收益最高者; `p == 1` 等价于在整个池上均匀随机。
 * @param gains 候选收益, 越大表示移除后成本下降越多
 * @param count 期望挑出的个数, 超过候选数时截断
 * @param noise_exponent 贪心指数 p, 调用方需保证 `>= 1`
 * @param random 随机源
 * @return 被选中元素在 `gains` 中的下标, 按挑选顺序
 */
std::vector<std::size_t> select_worst_removal_indices(const std::vector<double>& gains,
                                                      std::size_t count, double noise_exponent,
                                                      RandomUtils& random);

/**
 * @brief alns ruin算子最差订单移除
 * @details 按「把该 order 从所属 load 摘掉后目标值的下降量」排序, 随机化地优先移除最贵的订单
 */
class AlnsRuinWorstRemovalOrder : public AlnsRuinOperator {
 public:
  /**
   * @brief 随机种子
   */
  int select_order_random_seed{37};
  /**
   * @brief 随机数
   */
  RandomUtils::UPtr select_order_random;
  /**
   * @brief 选择订单数的最小比例
   */
  double select_order_min_rate{0.1};
  /**
   * @brief 选择订单数的最大比例
   */
  double select_order_max_rate{0.3};
  /**
   * @brief 贪心指数 p
   * @details 1 表示均匀随机, 越大越集中在收益最高的订单上
   */
  double select_order_noise_exponent{3.0};

  explicit AlnsRuinWorstRemovalOrder(const SolverContext* _context, const sol::table& cfg);

  [[nodiscard]] std::string format_parameters() const override;

  std::unordered_map<int, AlnsRuinLoadActivity::UPtr> execute(Solution* sol) override;
};
