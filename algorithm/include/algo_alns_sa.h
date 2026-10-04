/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "algo_alns_parameter.h"
#include "c_random_utils.h"
#include "pdm_solution.h"

/**
 * @brief alns 模拟退火 solution 接受的状态码
 */
enum class AlnsSaAcceptStatusCode {
  DOMINATE_BEST_SOLUTION,     // 优于 best solution
  DOMINATE_CURRENT_SOLUTION,  // 优于 current solution
  PROBABILITY_ACCEPT,         // 概率接受
  REJECT,                     // 不接受
};

/**
 * @brief 模拟退火
 * @details 用于判断是否接受解
 */
class AlnsSimulatedAnnealing {
 private:
  /**
   * @brief 最小温度
   */
  const double min_temperature;
  /**
   * @brief 冷却系数
   */
  const double annealing_factor;
  /**
   * @brief 随机数
   */
  const RandomUtils::UPtr accept_random;
  /**
   * @brief 温度
   */
  double temperature;

 public:
  explicit AlnsSimulatedAnnealing(const AlnsParameter* alns_parameter)
      : min_temperature(alns_parameter->min_temperature),
        annealing_factor(alns_parameter->annealing_factor),
        accept_random(std::make_unique<RandomUtils>(alns_parameter->default_random_seed)),
        temperature(alns_parameter->initial_temperature) {}

  ~AlnsSimulatedAnnealing() = default;
  /**
   * @brief 计算接受状态
   * @details 四种状态 优于最优解; 优于当前解; 概率接受; 不接受;
   * @param tmp_sol
   * @param cur_sol
   * @param best_sol
   * @return
   */
  AlnsSaAcceptStatusCode cal_accept_status(const Solution* tmp_sol, const Solution* cur_sol, const Solution* best_sol) const;
  /**
   * @brief 更新温度
   */
  void update_temperature();
};
