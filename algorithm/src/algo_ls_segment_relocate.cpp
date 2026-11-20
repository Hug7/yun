/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#include "algo_ls_segment_relocate.h"

#include <format>
#include <stdexcept>
#include <string>
#include <vector>

#include "algo_ls_neighborhood.h"
#include "pdm_solution.h"

namespace {
/**
 * @brief 由作用范围推导算子编码
 */
[[nodiscard]] LsOperatorCode to_operator_code(const LsRouteScope route_scope) {
  return route_scope == LsRouteScope::Intra ? LsOperatorCode::LsIntraRouteRelocate
                                            : LsOperatorCode::LsInterRouteRelocate;
}
}  // namespace

// ====== implement of LsSegmentRelocateOperator ======
LsSegmentRelocateOperator::LsSegmentRelocateOperator(const LsSegmentRelocateParam& _parameter)
    : LsOperator(to_operator_code(_parameter.route_scope)), parameter(_parameter) {
  if (this->parameter.min_chain_len < 1) {
    throw std::invalid_argument(this->code + ": `min_chain_len` must be >= 1, got " +
                                std::to_string(this->parameter.min_chain_len));
  }
  if (this->parameter.max_chain_len < this->parameter.min_chain_len) {
    throw std::invalid_argument(this->code + ": `max_chain_len` (" +
                                std::to_string(this->parameter.max_chain_len) +
                                ") must not be less than `min_chain_len` (" +
                                std::to_string(this->parameter.min_chain_len) + ")");
  }
}

void LsSegmentRelocateOperator::enumerate(const LsOperatorContext& context,
                                          std::vector<LsMove>& out) const {
  const Solution* sol = context.solution;
  const LsNeighborhood* neighborhood = context.neighborhood;
  if (neighborhood == nullptr) {
    return;
  }
  const bool intra_route = this->parameter.route_scope == LsRouteScope::Intra;
  const int load_count = static_cast<int>(sol->loads.size());
  for (int from_load = 0; from_load < load_count; ++from_load) {
    const int unit_count = neighborhood->movable_unit_count(*sol, from_load);
    if (unit_count <= 0) {
      continue;
    }
    for (int first_unit = 0; first_unit < unit_count; ++first_unit) {
      for (int chain_len = this->parameter.min_chain_len;
           chain_len <= this->parameter.max_chain_len; ++chain_len) {
        // 超出长度
        if (first_unit + chain_len > unit_count) {
          break;
        }
        const LsBlock block{
            .load_ind = from_load, .first_unit = first_unit, .unit_count = chain_len};
        if (intra_route) {
          for (const int to_slot : neighborhood->target_slots(*sol, block, from_load)) {
            // 同车插回原位是 no-op
            if (to_slot == first_unit) {
              continue;
            }
            out.emplace_back(LsRelocateMove{block, from_load, to_slot});
          }
          continue;
        }
        for (const int to_load : neighborhood->candidate_target_loads(*sol, block)) {
          for (const int to_slot : neighborhood->target_slots(*sol, block, to_load)) {
            out.emplace_back(LsRelocateMove{block, to_load, to_slot});
          }
        }
      }
    }
  }
}

std::string LsSegmentRelocateOperator::format_parameters() const {
  return std::format("min_chain_len={}, max_chain_len={}", this->parameter.min_chain_len,
                     this->parameter.max_chain_len);
}
