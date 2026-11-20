/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#include "algo_ls_exchange.h"

#include <vector>

#include "algo_ls_neighborhood.h"
#include "pdm_solution.h"

namespace {
/**
 * @brief 由作用范围推导算子编码
 */
[[nodiscard]] LsOperatorCode to_operator_code(const LsRouteScope route_scope) {
  return route_scope == LsRouteScope::Intra ? LsOperatorCode::LsIntraRouteExchange
                                            : LsOperatorCode::LsInterRouteExchange;
}
}  // namespace

// ====== implement of LsExchangeOperator ======
LsExchangeOperator::LsExchangeOperator(const LsExchangeParam& _parameter)
    : LsOperator(to_operator_code(_parameter.route_scope)), parameter(_parameter) {}

void LsExchangeOperator::enumerate(const LsOperatorContext& context,
                                   std::vector<LsMove>& out) const {
  const Solution* sol = context.solution;
  const LsNeighborhood* neighborhood = context.neighborhood;
  if (sol == nullptr || neighborhood == nullptr) {
    return;
  }
  const bool intra_route = this->parameter.route_scope == LsRouteScope::Intra;
  const int load_count = static_cast<int>(sol->loads.size());
  for (int first_load = 0; first_load < load_count; ++first_load) {
    const int first_unit_count = neighborhood->movable_unit_count(*sol, first_load);
    if (first_unit_count <= 0) {
      continue;
    }
    for (int first_unit = 0; first_unit < first_unit_count; ++first_unit) {
      const LsBlock first{.load_ind = first_load, .first_unit = first_unit, .unit_count = 1};
      if (intra_route) {
        // 同车只枚举 second_unit > first_unit 的无序对
        for (int second_unit = first_unit + 1; second_unit < first_unit_count; ++second_unit) {
          const LsBlock second{.load_ind = first_load, .first_unit = second_unit, .unit_count = 1};
          out.emplace_back(LsExchangeMove{first, second});
        }
        continue;
      }
      for (const int second_load : neighborhood->candidate_target_loads(*sol, first)) {
        // 跨车只枚举 second_load > first_load 的无序对
        if (second_load <= first_load) {
          continue;
        }
        const int second_unit_count = neighborhood->movable_unit_count(*sol, second_load);
        for (int second_unit = 0; second_unit < second_unit_count; ++second_unit) {
          const LsBlock second{.load_ind = second_load, .first_unit = second_unit, .unit_count = 1};
          out.emplace_back(LsExchangeMove{first, second});
        }
      }
    }
  }
}

std::string LsExchangeOperator::format_parameters() const { return ""; }
