/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#include "algo_alns_solution_manager.h"

// ====== implement of AlnsSolutionManager ======
AlnsSolutionManager::AlnsSolutionManager(const Workspace* _workspace,
                                         const AlnsParameter* _parameter) {
  this->workspace = _workspace;
  this->best_sol = this->workspace->generate_sol();
  this->cur_sol = this->workspace->deep_copy_sol(this->best_sol);
  this->sa = new AlnsSimulatedAnnealing(_parameter);
}

AlnsSolutionManager::~AlnsSolutionManager() {
  delete this->best_sol;
  delete this->cur_sol;
  delete this->sa;
}

void AlnsSolutionManager::reset_cur_sol() {
  delete this->cur_sol;
  this->cur_sol = this->workspace->deep_copy_sol(this->best_sol);
}

AlnsSaAcceptStatusCode AlnsSolutionManager::update_sol(const Solution* tmp_sol) {
  const auto status_code = this->sa->cal_accept_status(tmp_sol, this->cur_sol, this->best_sol);

  if (status_code == AlnsSaAcceptStatusCode::DOMINATE_BEST_SOLUTION) {
    delete this->best_sol;
    this->best_sol = this->workspace->deep_copy_sol(tmp_sol);
    delete this->cur_sol;
    this->cur_sol = this->workspace->deep_copy_sol(tmp_sol);
  } else if (status_code == AlnsSaAcceptStatusCode::DOMINATE_BEST_SOLUTION) {
    delete this->cur_sol;
    this->cur_sol = this->workspace->deep_copy_sol(tmp_sol);
  } else if (status_code == AlnsSaAcceptStatusCode::REJECT) {
    delete tmp_sol;
    tmp_sol = this->workspace->deep_copy_sol(this->cur_sol);
  }

  return status_code;
}