/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <spdlog/spdlog.h>

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
 * @brief 解析目标值尺度 f_ref
 * @details 取初始解总目标值的绝对值。目标值本身可能为 0 或负值，直接做分母会除零或翻转
 *          接受概率的符号，因此退化时回落到 1.0
 * @param init_total_cost 初始解的总目标值 f0
 * @return f_ref > 0
 */
[[nodiscard]] double resolve_obj_scale(double init_total_cost);

/**
 * @brief 相对温度 Metropolis 接受概率
 * @details
 *   P = 1                                   (delta <= 0)
 *   P = exp(-delta / (tau * f_ref))          (delta > 0)
 * 等价于绝对温度形式 exp(-delta / T_abs)，其中 T_abs = tau * f_ref。
 * 分子 delta / f_ref 是无量纲的相对劣化率，因此 tau 也是无量纲量，跨算例可复用
 * @param delta 目标值增量 f_tmp - f_cur
 * @param obj_scale 目标值尺度 f_ref
 * @param relative_temperature 相对温度 tau > 0
 */
[[nodiscard]] double metropolis_accept_probability(double delta, double obj_scale,
                                                   double relative_temperature);

/**
 * @brief 由首末温度与迭代次数反解冷却系数
 * @details alpha = (tau_end / tau_0) ^ (1 / iters)，保证第 iters 轮恰好降到 tau_end
 * @param initial_relative_temperature 初始相对温度 tau_0
 * @param min_relative_temperature 最小相对温度 tau_end
 * @param iters 降温次数（通常为 max_iter）
 */
[[nodiscard]] double resolve_annealing_factor(double initial_relative_temperature,
                                              double min_relative_temperature, int iters);

/**
 * @brief 模拟退火
 * @details 使用相对温度的 metropolis 准则判断是否接受解；温度参数无量纲，
 *          不随算例目标值量级变化
 */
class AlnsSimulatedAnnealing {
 private:
  /**
   * @brief 最小相对温度 tau_end
   */
  const double min_relative_temperature;
  /**
   * @brief 冷却系数 alpha
   */
  const double annealing_factor;
  /**
   * @brief 随机数
   */
  const RandomUtils::UPtr accept_random;
  /**
   * @brief 目标值尺度 f_ref = |f0|
   * @details 引入初解的总目标值作为归一化基准，使温度不依赖目标函数的量级
   */
  const double obj_scale;
  /**
   * @brief 日志器
   * @details 用于输出 delta 分布，供 tau_0 标定；可为空
   */
  const std::shared_ptr<spdlog::logger> logger;
  /**
   * @brief 相对温度 tau
   */
  double relative_temperature;

 public:
  explicit AlnsSimulatedAnnealing(const AlnsParameter* alns_parameter, const Solution* init_sol,
                                  std::shared_ptr<spdlog::logger> _logger);

  ~AlnsSimulatedAnnealing() = default;
  /**
   * @brief 计算接受状态
   * @details 四种状态 优于最优解; 优于当前解; 概率接受; 不接受;
   * @param tmp_sol
   * @param cur_sol
   * @param best_sol
   * @return
   */
  AlnsSaAcceptStatusCode cal_accept_status(const Solution* tmp_sol, const Solution* cur_sol,
                                           const Solution* best_sol) const;
  /**
   * @brief 更新温度
   */
  void update_temperature();

  /**
   * @brief 当前相对温度
   */
  [[nodiscard]] double current_temperature() const { return this->relative_temperature; }
  /**
   * @brief 目标值尺度 f_ref
   */
  [[nodiscard]] double objective_scale() const { return this->obj_scale; }
};
