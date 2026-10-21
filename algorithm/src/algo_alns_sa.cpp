/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#include "algo_alns_sa.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <utility>

#include "algo_alns_parameter.h"

namespace {
/**
 * @brief 目标值尺度的下限
 * @details |f0| 小于该值时认为目标值退化为 0，f_ref 回落到 1.0
 */
constexpr double kObjScaleFloor = 1e-9;
}  // namespace

// ====== implement of SA 纯函数 ======
double resolve_obj_scale(const double init_total_cost) {
  const double scale = std::abs(init_total_cost);
  return scale > kObjScaleFloor ? scale : 1.0;
}

double metropolis_accept_probability(const double delta, const double obj_scale,
                                     const double relative_temperature) {
  if (delta <= 0.0) {
    return 1.0;
  }
  if (obj_scale <= 0.0 || relative_temperature <= 0.0) {
    return 0.0;
  }
  // P = exp(-delta / (tau * f_ref))
  return std::exp(-delta / (relative_temperature * obj_scale));
}

double resolve_annealing_factor(const double initial_relative_temperature,
                                const double min_relative_temperature, const int iters) {
  if (iters <= 0 || initial_relative_temperature <= 0.0 || min_relative_temperature <= 0.0 ||
      min_relative_temperature >= initial_relative_temperature) {
    return 1.0;
  }
  // alpha^iters * tau_0 = tau_end
  return std::pow(min_relative_temperature / initial_relative_temperature,
                  1.0 / static_cast<double>(iters));
}

// ====== implement of AlnsSimulatedAnnealing ======
AlnsSimulatedAnnealing::AlnsSimulatedAnnealing(const AlnsParameter* alns_parameter,
                                               const Solution* init_sol,
                                               std::shared_ptr<spdlog::logger> _logger)
    : min_relative_temperature(alns_parameter->min_temperature),
      annealing_factor(alns_parameter->annealing_factor > 0.0
                           ? alns_parameter->annealing_factor
                           : resolve_annealing_factor(alns_parameter->initial_temperature,
                                                      alns_parameter->min_temperature,
                                                      alns_parameter->max_iter)),
      accept_random(std::make_unique<RandomUtils>(alns_parameter->default_random_seed)),
      obj_scale(init_sol != nullptr ? resolve_obj_scale(init_sol->total_cost()) : 1.0),
      logger(std::move(_logger)),
      relative_temperature(alns_parameter->initial_temperature) {
  if (this->relative_temperature <= 0.0) {
    throw std::invalid_argument("AlnsSimulatedAnnealing: `initial_temperature` must be > 0");
  }
  if (this->min_relative_temperature <= 0.0) {
    throw std::invalid_argument("AlnsSimulatedAnnealing: `min_temperature` must be > 0");
  }
  if (this->min_relative_temperature > this->relative_temperature) {
    throw std::invalid_argument(
        "AlnsSimulatedAnnealing: `min_temperature` must not exceed `initial_temperature`");
  }
  if (this->logger != nullptr) {
    if (init_sol == nullptr) {
      this->logger->warn(
          "AlnsSimulatedAnnealing: init solution is null, objective scale falls back to 1.0");
    } else {
      this->logger->debug(
          "AlnsSimulatedAnnealing: f0={:.6f} obj_scale={:.6f} tau0={:.6e} tau_min={:.6e} "
          "annealing_factor={:.6f}",
          init_sol->total_cost(), this->obj_scale, this->relative_temperature,
          this->min_relative_temperature, this->annealing_factor);
    }
  }
}

AlnsSaAcceptStatusCode AlnsSimulatedAnnealing::cal_accept_status(const Solution* tmp_sol,
                                                                 const Solution* cur_sol,
                                                                 const Solution* best_sol) const {
  const double tmp_cost = tmp_sol->total_cost();
  const double cur_cost = cur_sol->total_cost();
  if (tmp_cost < best_sol->total_cost()) {
    return AlnsSaAcceptStatusCode::DOMINATE_BEST_SOLUTION;
  }
  const double delta = tmp_cost - cur_cost;
  if (delta < 0.0) {
    return AlnsSaAcceptStatusCode::DOMINATE_CURRENT_SOLUTION;
  }
  if (delta == 0.0) {
    return AlnsSaAcceptStatusCode::PROBABILITY_ACCEPT;
  }
  // 相对温度 metropolis 准则: P = exp(-delta / (tau * f0))
  const double accept_probability =
      metropolis_accept_probability(delta, this->obj_scale, this->relative_temperature);
  // if (this->logger != nullptr) {
  //   // 打印正劣化的相对量 r = delta / f0，用于标定 tau_0
  //   this->logger->debug("sa metropolis: delta={:.6f} r={:.6e} tau={:.6e} p={:.6f}", delta,
  //                       delta / this->obj_scale, this->relative_temperature, accept_probability);
  // }
  if (this->accept_random->random_double() < accept_probability) {
    return AlnsSaAcceptStatusCode::PROBABILITY_ACCEPT;
  }
  return AlnsSaAcceptStatusCode::REJECT;
}

void AlnsSimulatedAnnealing::update_temperature() {
  this->relative_temperature =
      std::max(this->relative_temperature * this->annealing_factor, this->min_relative_temperature);
}
