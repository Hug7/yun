/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#include "algo_alns_sa.h"

#include <cmath>

// ====== implement of AlnsSimulatedAnnealing ======
AlnsSaAcceptStatusCode AlnsSimulatedAnnealing::cal_accept_status(const Solution* tmp_sol,
                                                      const Solution* cur_sol,
                                                      const Solution* best_sol) const {
  if (tmp_sol->total_cost() < best_sol->total_cost()) {
    return AlnsSaAcceptStatusCode::DOMINATE_BEST_SOLUTION;
  }
  const double delta = tmp_sol->total_cost() - cur_sol->total_cost();
  if (delta < 0) {
    return AlnsSaAcceptStatusCode::DOMINATE_CURRENT_SOLUTION;
  }
  // 使用 metropolis 准则计算接受概率
  double accept_probability = 1.0;
  if (delta > 0) {
    accept_probability = std::exp(-delta / this->temperature);
  }
  if (this->accept_random->random_double() < accept_probability) {
    return AlnsSaAcceptStatusCode::PROBABILITY_ACCEPT;
  }

  return AlnsSaAcceptStatusCode::REJECT;
}

void AlnsSimulatedAnnealing::update_temperature() {
  this->temperature *= this->annealing_factor;
  this->temperature = std::max(this->temperature, this->min_temperature);
}