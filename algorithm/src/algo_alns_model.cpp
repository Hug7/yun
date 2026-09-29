/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#include "algo_alns_model.h"

// ====== implement of AlnsSolutionManager ======
AlnsSolutionManager::AlnsSolutionManager(const Workspace* _workspace) {
  this->best_sol = _workspace->generate_sol();
  this->cur_sol = _workspace->generate_sol();
  this->tmp_sol = _workspace->generate_sol();
}

AlnsSolutionManager::~AlnsSolutionManager() {
  delete this->best_sol;
  delete this->cur_sol;
  delete this->tmp_sol;
}

// ====== implement of AlnsModel ======
AlnsModel::AlnsModel(const Workspace* _workspace, const AlnsParameter* _parameter,
                     const AlnsOperatorManager* operator_manager)
    : workspace(_workspace),
      parameter(_parameter),
      operator_manager(operator_manager),
      sol_manager(new AlnsSolutionManager(workspace)) {}
