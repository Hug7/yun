/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <sol/sol.hpp>

#include "algo_alns_operator.h"
#include "algo_alns_parameter.h"
#include "pdm_solution.h"
#include "pdm_workspace.h"

class AlnsSolutionManager {
 public:
  /**
   * @brief 最优解
   */
  Solution* best_sol;
  /**
   * @brief 当前最优解
   */
  Solution* cur_sol;
  /**
   * @brief 临时解
   */
  Solution* tmp_sol;

  explicit AlnsSolutionManager(const Workspace* _workspace);

  ~AlnsSolutionManager();
};

class AlnsModel {
 public:
  const Workspace* workspace;

  const AlnsParameter* parameter;

  const AlnsOperatorManager* operator_manager;

  const AlnsSolutionManager* sol_manager;

  explicit AlnsModel(const Workspace* _workspace, const AlnsParameter* _parameter,
                     const AlnsOperatorManager* operator_manager);
};
