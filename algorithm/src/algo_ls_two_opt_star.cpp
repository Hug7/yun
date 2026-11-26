/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#include "algo_ls_two_opt_star.h"

#include <format>
#include <stdexcept>
#include <string>
#include <vector>

#include "algo_ls_neighborhood.h"
#include "pdm_solution.h"

namespace {
/**
 * @brief 该车次在参数约束下可用的最长尾部段长
 */
[[nodiscard]] int max_tail_len_of(const int unit_count, const int configured_max) {
  return configured_max == 0 || configured_max > unit_count ? unit_count : configured_max;
}
}  // namespace

// ====== implement of LsTwoOptStarOperator ======
LsTwoOptStarOperator::LsTwoOptStarOperator(const LsTwoOptStarParam& _parameter)
    : LsOperator(LsOperatorCode::LsTwoOptStar), parameter(_parameter) {
  if (this->parameter.min_tail_len < 1) {
    throw std::invalid_argument(this->code + ": `min_tail_len` must be >= 1, got " +
                                std::to_string(this->parameter.min_tail_len));
  }
  if (this->parameter.max_tail_len != 0 &&
      this->parameter.max_tail_len < this->parameter.min_tail_len) {
    throw std::invalid_argument(this->code + ": `max_tail_len` (" +
                                std::to_string(this->parameter.max_tail_len) +
                                ") must be 0 (unbounded) or >= `min_tail_len` (" +
                                std::to_string(this->parameter.min_tail_len) + ")");
  }
}

void LsTwoOptStarOperator::enumerate(const LsOperatorContext& context,
                                     std::vector<LsMove>& out) const {
  const Solution* sol = context.solution;
  const LsNeighborhood* neighborhood = context.neighborhood;

  const int load_count = static_cast<int>(sol->loads.size());
  for (int first_load = 0; first_load < load_count; ++first_load) {
    const int first_unit_count = neighborhood->movable_unit_count(*sol, first_load);
    const int first_max_tail = max_tail_len_of(first_unit_count, this->parameter.max_tail_len);
    if (first_max_tail < this->parameter.min_tail_len) {
      continue;
    }
    // 目标车次只取决于源车次的 PICK 站点，与具体尾部段无关：取长度 1 的尾部探针即可
    const LsBlock probe{
        .load_ind = first_load, .first_unit = first_unit_count - 1, .unit_count = 1};
    for (const int second_load : neighborhood->candidate_target_loads(*sol, probe)) {
      // 只枚举 second_load > first_load 的无序车次对
      if (second_load <= first_load) {
        continue;
      }
      const int second_unit_count = neighborhood->movable_unit_count(*sol, second_load);
      const int second_max_tail = max_tail_len_of(second_unit_count, this->parameter.max_tail_len);
      if (second_max_tail < this->parameter.min_tail_len) {
        continue;
      }
      for (int first_tail_len = this->parameter.min_tail_len; first_tail_len <= first_max_tail;
           ++first_tail_len) {
        const LsBlock first{.load_ind = first_load,
                            .first_unit = first_unit_count - first_tail_len,
                            .unit_count = first_tail_len};
        for (int second_tail_len = this->parameter.min_tail_len; second_tail_len <= second_max_tail;
             ++second_tail_len) {
          const LsBlock second{.load_ind = second_load,
                               .first_unit = second_unit_count - second_tail_len,
                               .unit_count = second_tail_len};
          out.emplace_back(LsExchangeMove{first, second});
        }
      }
    }
  }
}

std::string LsTwoOptStarOperator::format_parameters() const {
  return std::format("min_tail_len={}, max_tail_len={}", this->parameter.min_tail_len,
                     this->parameter.max_tail_len);
}
