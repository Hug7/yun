/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <memory>
#include <sol/sol.hpp>
#include <string>
#include <vector>

#include "algo_alns_parameter.h"
#include "algo_alns_repair.h"
#include "algo_alns_ruin.h"
#include "algo_alns_sa.h"
#include "c_random_utils.h"
#include "pdm_context.h"

/**
 * @brief alns ruin×repair 组合的统计
 * @details 仅用于观测, 不参与权重计算
 */
class AlnsOperatorStat {
 public:
  /**
   * @brief 算子分数累计
   */
  double score_sum{0};
  /**
   * @brief 使用次数
   */
  int usage{0};
  /**
   * @brief 平均分, usage 为 0 时返回 0
   */
  [[nodiscard]] double avg() const;
};

/**
 * @brief alns 算子的组合
 */
class AlnsOperatorCombination {
 public:
  using UPtr = std::unique_ptr<AlnsOperatorCombination>;
  /**
   * @brief alns ruin 算子索引
   */
  const int ruin_operator_ind;
  /**
   * @brief alns repair 算子索引
   */
  const int repair_operator_ind;
  /**
   * @brief 算子分数
   */
  double operator_score{0};

  explicit AlnsOperatorCombination(const int _ruin_operator_ind, const int _repair_operator_ind)
      : ruin_operator_ind(_ruin_operator_ind), repair_operator_ind(_repair_operator_ind) {};
  /**
   * @brief 计算算子状态码的分数
   * @param status_code 接受解的状态码
   */
  void cal_operator_score(const AlnsSaAcceptStatusCode& status_code);
};

class AlnsOperatorManager {
 public:
  /**
   * @brief 默认的ruin算子编码集合
   * @details 元素顺序即默认注册顺序, 保证求解前打印与轮盘下标稳定
   */
  inline static const std::vector<AlnsRuinOperatorCode> default_ruin_operator_codes{
      AlnsRuinOperatorCode::RuinRandomRemovalLoad,
      AlnsRuinOperatorCode::RuinRandomRemovalOrder,
      AlnsRuinOperatorCode::RuinWorstRemovalOrder,
  };
  /**
   * @brief 默认的repair算子编码集合
   * @details 元素顺序即默认注册顺序, 保证求解前打印与轮盘下标稳定
   */
  inline static const std::vector<AlnsRepairOperatorCode> default_repair_operator_codes{
      AlnsRepairOperatorCode::RepairGreedy,
      AlnsRepairOperatorCode::RepairRegretK,
  };
  /**
   * @brief alns 的算法参数
   */
  const AlnsParameter* parameter;
  /**
   * @brief ruin算子列表（按实例顺序，同算子可多实例不同参数）
   */
  std::vector<AlnsRuinOperator*> ruin_operators;
  /**
   * @brief ruin算子数量
   */
  int ruin_operator_count{0};
  /**
   * @brief repair算子列表（按实例顺序）
   */
  std::vector<AlnsRepairOperator*> repair_operators;
  /**
   * @brief repair算子数量
   */
  int repair_operator_count{0};
  /**
   * @brief ruin 算子权重（按实例顺序，与 ruin_operators 一一对应）
   */
  std::vector<double> ruin_operator_weights;
  /**
   * @brief repair 算子权重（按实例顺序，与 repair_operators 一一对应）
   */
  std::vector<double> repair_operator_weights;
  /**
   * @brief ruin×repair 组合的二维统计矩阵 [ruin_idx][repair_idx]
   * @details 整轮累计, 不参与权重更新
   */
  std::vector<std::vector<AlnsOperatorStat>> operator_combination_stats;
  /**
   * @brief 选取算子的随机数
   */
  RandomUtils* select_operator_random;

  explicit AlnsOperatorManager(const AlnsParameter* _alns_parameter)
      : parameter(_alns_parameter),
        select_operator_random(new RandomUtils(_alns_parameter->default_random_seed)) {};

  ~AlnsOperatorManager();

  void add_ruin_operator(AlnsRuinOperator* ruin_operator);

  void add_repair_operator(AlnsRepairOperator* repair_operator);

  /**
   * @brief 初始化权重与统计矩阵
   * @details 必须在所有算子注册完成后调用, 且 ruin/repair 算子都不能为空
   */
  void init_weights();

  [[nodiscard]] AlnsRuinOperator* ruin_operator(
      const AlnsOperatorCombination::UPtr& operator_combination) const;

  [[nodiscard]] AlnsRepairOperator* repair_operator(
      const AlnsOperatorCombination::UPtr& operator_combination) const;

  [[nodiscard]] std::vector<AlnsOperatorCombination::UPtr> roulette_wheel_operator(
      int turned_count) const;

  void update_operator_weights(
      const std::vector<AlnsOperatorCombination::UPtr>& operator_combinations);

  /**
   * @brief 格式化求解时使用的算子及其参数（不含结尾换行）
   */
  [[nodiscard]] std::string format_operators() const;

  /**
   * @brief 格式化算子统计文本（不含结尾换行）
   */
  [[nodiscard]] std::string format_statistics() const;
};

/**
 * @brief 创建alns算子(自定义)
 * @param cfg 算子集合
 * @param alns_parameter alns参数
 * @param context solver上下文
 * @param parallel_executor 线程池, 由调用方持有, repair算子借用其并行构造修复矩阵
 * @return
 */
std::unique_ptr<AlnsOperatorManager> create_alns_operator(const sol::table& cfg,
                                                          const AlnsParameter* alns_parameter,
                                                          const SolverContext* context,
                                                          ParallelExecutor* parallel_executor);
