/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#include "algo_ls_two_opt.h"

#include <format>
#include <stdexcept>
#include <string>
#include <vector>

#include "algo_ls_neighborhood.h"
#include "pdm_solution.h"

// ====== implement of LsTwoOptOperator ======
LsTwoOptOperator::LsTwoOptOperator(const LsTwoOptParam& _parameter)
    : LsOperator(LsOperatorCode::LsTwoOpt), parameter(_parameter) {
  if (this->parameter.min_span_len < 2) {
    throw std::invalid_argument(this->code + ": `min_span_len` must be >= 2, got " +
                                std::to_string(this->parameter.min_span_len));
  }
  if (this->parameter.max_span_len != 0 &&
      this->parameter.max_span_len < this->parameter.min_span_len) {
    throw std::invalid_argument(this->code + ": `max_span_len` (" +
                                std::to_string(this->parameter.max_span_len) +
                                ") must be 0 (unbounded) or >= `min_span_len` (" +
                                std::to_string(this->parameter.min_span_len) + ")");
  }
}

void LsTwoOptOperator::enumerate(const LsOperatorContext& context, std::vector<LsMove>& out) const {
  const Solution* sol = context.solution;
  const LsNeighborhood* neighborhood = context.neighborhood;
  if (sol == nullptr || neighborhood == nullptr) {
    return;
  }
  const int load_count = static_cast<int>(sol->loads.size());
  for (int load_ind = 0; load_ind < load_count; ++load_ind) {
    const int unit_count = neighborhood->movable_unit_count(*sol, load_ind);
    if (unit_count <= 0) {
      continue;
    }
    const int max_span_len =
        this->parameter.max_span_len == 0 ? unit_count : this->parameter.max_span_len;
    for (int first_unit = 0; first_unit < unit_count; ++first_unit) {
      for (int span_len = this->parameter.min_span_len; span_len <= max_span_len; ++span_len) {
        if (first_unit + span_len > unit_count) {
          break;
        }
        out.emplace_back(LsReverseMove{
            LsBlock{.load_ind = load_ind, .first_unit = first_unit, .unit_count = span_len}});
      }
    }
  }
}

std::string LsTwoOptOperator::format_parameters() const {
  return std::format("min_span_len={}, max_span_len={}", this->parameter.min_span_len,
                     this->parameter.max_span_len);
}
