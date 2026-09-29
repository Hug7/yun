/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <sol/sol.hpp>
#include <unordered_set>
#include <vector>

#include "algo_alns_parameter.h"
#include "algo_alns_repair.h"
#include "algo_alns_ruin.h"
#include "c_random_utils.h"
#include "pdm_workspace.h"

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
   * @brief ruin算子列表（按实例顺序，同算子可多实例不同参数）
   */
  std::vector<AlnsRuinOperator*> ruin_operators;
  /**
   * @brief repair算子列表（按实例顺序）
   */
  std::vector<AlnsRepairOperator*> repair_operators;
  /**
   * @brief ruin和repair算子的组合权重 [ruin_idx][repair_idx]
   */
  std::vector<std::vector<double>> operator_weights;
  /**
   * @brief 选取算子的随机数
   */
  RandomUtils* select_operator_random{};

  AlnsOperatorManager() = default;

  ~AlnsOperatorManager();
};

/**
 * @brief 创建alns算子(自定义)
 * @param cfg 算子集合
 * @param alns_parameter alns参数
 * @param workspace lua中定义的alns参数
 * @return
 */
AlnsOperatorManager* create_alns_operator(const sol::table& cfg, const AlnsParameter* alns_parameter,
                                          const Workspace* workspace);
