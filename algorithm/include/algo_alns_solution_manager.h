/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "algo_alns_parameter.h"
#include "algo_alns_sa.h"
#include "pdm_solution.h"
#include "pdm_workspace.h"

class AlnsSolutionManager {
 public:
  /**
   * @brief 工作空间
   */
  const Workspace* workspace;
  /**
   * @brief 最优解
   */
  Solution* best_sol;
  /**
   * @brief 当前解
   */
  Solution* cur_sol;
  /**
   * @brief 临时解
   */
  Solution* tmp_sol;
  /**
   * @brief 模拟退火-判断是否接收解
   */
  AlnsSimulatedAnnealing* sa;

  explicit AlnsSolutionManager(const Workspace* _workspace, const AlnsParameter* _parameter);

  ~AlnsSolutionManager();
  /**
   * @brief 重置 current solution
   * @details 将 best solution 重新赋值给 current solution
   */
  void reset_cur_sol();
  /**
   * @brief 重置 temp solution
   * @details 将 current solution 重新赋值给 temp solution
   */
  void reset_tmp_sol();
  /**
   * @brief 使用 tmp_sol 更新 cur_sol 和 best_sol
   * @return solution 的接受状态码
   */
  [[nodiscard]] AlnsSaAcceptStatusCode update_sol();
};
