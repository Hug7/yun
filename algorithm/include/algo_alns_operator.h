/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <memory>
#include <sol/sol.hpp>
#include <unordered_set>
#include <vector>

#include "algo_alns_parameter.h"
#include "algo_alns_repair.h"
#include "algo_alns_ruin.h"
#include "algo_alns_sa.h"
#include "c_random_utils.h"
#include "pdm_workspace.h"

/**
 * @brief alns 算子的组合
 */
class AlnsOperatorCombination {
 public:
  using UPtr = std::unique_ptr<AlnsOperatorCombination>;
  /**
   * @brief ruin 和 repair 组合算子的索引
   */
  const int operator_combination_ind;
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

  explicit AlnsOperatorCombination(const int _operator_combination_ind,
                                   const int _ruin_operator_ind, const int _repair_operator_ind)
      : operator_combination_ind(_operator_combination_ind),
        ruin_operator_ind(_ruin_operator_ind),
        repair_operator_ind(_repair_operator_ind) {};
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
   */
  inline static std::unordered_set<AlnsRuinOperatorCode> default_ruin_operator_codes{
      AlnsRuinOperatorCode::RuinRandomLoad,
  };
  /**
   * @brief 默认的repair算子编码集合
   */
  inline static std::unordered_set<AlnsRepairOperatorCode> default_repair_operator_codes{
      AlnsRepairOperatorCode::RepairGreedy,
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
   * @brief ruin和repair算子组合权重的展开 [ruin_idx][repair_idx]
   */
  std::vector<double> operator_combination_weights;
  /**
   * @brief 算子组合的数量
   */
  int operator_combination_count{0};
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

  [[nodiscard]] AlnsRuinOperator* ruin_operator(
      const AlnsOperatorCombination::UPtr& operator_combination) const;

  [[nodiscard]] AlnsRepairOperator* repair_operator(
      const AlnsOperatorCombination::UPtr& operator_combination) const;

  [[nodiscard]] std::vector<AlnsOperatorCombination::UPtr> roulette_wheel_operator(
      int turned_count) const;

  void update_operator_weights(
      const std::vector<AlnsOperatorCombination::UPtr>& operator_combinations);
};

/**
 * @brief 创建alns算子(自定义)
 * @param cfg 算子集合
 * @param alns_parameter alns参数
 * @param workspace lua中定义的alns参数
 * @param thread_pool 线程池, 由调用方持有, repair算子借用其并行构造修复矩阵
 * @return
 */
AlnsOperatorManager* create_alns_operator(const sol::table& cfg,
                                          const AlnsParameter* alns_parameter,
                                          const Workspace* workspace, ThreadPool* thread_pool);
