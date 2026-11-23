/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <cstdint>

/**
 * @brief ALNS 算法的参数
 */
class AlnsParameter {
 public:
  /**
   * @brief 并行数
   * @details -1 表示自动使用默认硬件并发; 0 与其他负数非法
   */
  int worker_count{-1};
  /**
   * @brief 最大迭代次数
   */
  int max_iter{200};
  /**
   * @brief 最大子迭代次数
   */
  int max_sub_iter{10};
  /**
   * @brief 最大无改进次数
   */
  int max_unimproved_iter{100};
  /**
   * @brief 最大停滞迭代次数
   * @details 最优解超过`最大停滞迭代次数`没有更新，则触发重置current solution和temp solution
   */
  int max_stagnate_iter{20};
  /**
   * @brief 打印日志的间隔代数
   */
  int print_log_interval_iter{10};
  /**
   * @brief 最大运行时长
   */
  std::int64_t max_running_time_sec{600};
  /**
   * @brief 算子权重的反应因子 ρ (Ropke-Pisinger)
   * @details 按 `operator_weight_update_interval_iter` 分段更新:
   *          w_i <- (1 - ρ) * w_i + ρ * (π_i / θ_i)
   */
  double operator_learning_factor{0.35};
  /**
   * @brief 算子初始权重
   * @details 与 σ 分数同量级, 避免初值在前若干轮主导算子选择
   */
  double operator_initial_weight{1.0};
  /**
   * @brief 算子权重下限
   * @details 权重自适应后取 max(weight, operator_min_weight), 避免算子被永久淘汰
   */
  double operator_min_weight{0.05};
  /**
   * @brief 算子权重的更新间隔 (外层迭代数)
   * @details 每个 segment 内累计各算子的得分与使用次数, 达到该轮数后统一更新一次权重。
   *          默认 5 是相对 max_iter=100~200 的短程搜索标定的: 间隔过大会让劣质算子
   *          在前若干个 segment 仍分到过多预算
   */
  int operator_weight_update_interval_iter{5};
  /**
   * @brief 是否在 alns 运行结束后打印算子统计
   */
  bool print_operator_statistics{false};
  /**
   * @brief 初始相对温度 tau_0
   * @details 无量纲。接受概率为 exp(-delta / (tau * f0))，f0 是初始解的总目标值，
   *          因此该值不随算例目标值的量级变化，无需按算例重新标定
   */
  double initial_temperature{2.0e-2};
  /**
   * @brief 最小相对温度 tau_end
   * @details 无量纲。取到该值后温度不再下降，退化为近似贪心
   */
  double min_temperature{2.0e-5};
  /**
   * @brief 冷却系数 alpha
   * @details 每轮外层迭代温度乘以 alpha。设置为 <= 0 时，由
   *          alpha = (tau_end / tau_0) ^ (1 / max_iter) 自动推导，
   *          保证最后一轮恰好降到 tau_end
   */
  double annealing_factor{0.0};
  /**
   * @brief 默认的随机种子
   */
  int default_random_seed{37};

  AlnsParameter() = default;

  ~AlnsParameter() = default;
};
