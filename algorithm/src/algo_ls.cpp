/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#include "algo_ls.h"

#include <format>
#include <memory>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

#include "algo_ls_search.h"
#include "pdm_context.h"

// ====== implement of LsReport ======
std::string LsReport::format() const {
  std::string text = std::format(
      "local_search: obj {:.2f} -> {:.2f} (delta {:.2f}), passes={}, generated={}, pruned={}, "
      "dist_pruned={}, evaluated={}, feasible={}, applied={}, elapsed={}ms, improvement={}, "
      "reason={}",
      this->initial_obj, this->final_obj, this->delta_obj, this->passes, this->generated,
      this->pruned, this->dist_pruned, this->evaluated, this->feasible, this->applied,
      this->elapsed_ms, this->improvement, this->termination_reason);
  for (const auto& stat : this->operator_stats) {
    text += std::format(
        "\n  op[{}]={} generated={} pruned={} dist_pruned={} evaluated={} feasible={} "
        "applied={} delta={:.2f}",
        stat.instance_index, stat.code, stat.generated, stat.pruned, stat.dist_pruned,
        stat.evaluated, stat.feasible, stat.applied, stat.delta_obj);
  }
  return text;
}

// ====== implement of local search entry ======
std::vector<LsOperatorParam> default_ls_operator_params() {
  return {
      LsSegmentRelocateParam{
          .min_chain_len = 1, .max_chain_len = 1, .route_scope = LsRouteScope::Intra},
      LsSegmentRelocateParam{
          .min_chain_len = 2, .max_chain_len = 3, .route_scope = LsRouteScope::Intra},
      LsExchangeParam{.route_scope = LsRouteScope::Intra},
      LsSegmentRelocateParam{
          .min_chain_len = 1, .max_chain_len = 1, .route_scope = LsRouteScope::Inter},
      LsSegmentRelocateParam{
          .min_chain_len = 2, .max_chain_len = 3, .route_scope = LsRouteScope::Inter},
      LsExchangeParam{.route_scope = LsRouteScope::Inter},
  };
}

std::vector<std::unique_ptr<LsOperator>> create_ls_operators(
    const std::vector<LsOperatorParam>& operator_params) {
  // 为空时使用默认算子
  const std::vector<LsOperatorParam> params =
      operator_params.empty() ? default_ls_operator_params() : operator_params;
  // 实例化算子
  std::vector<std::unique_ptr<LsOperator>> operators;
  operators.reserve(params.size());
  for (const auto& param : params) {
    std::visit(
        [&operators](const auto& operator_param) {
          using ParamType = std::decay_t<decltype(operator_param)>;
          if constexpr (std::is_same_v<ParamType, LsSegmentRelocateParam>) {
            operators.emplace_back(std::make_unique<LsSegmentRelocateOperator>(operator_param));
          } else if constexpr (std::is_same_v<ParamType, LsExchangeParam>) {
            operators.emplace_back(std::make_unique<LsExchangeOperator>(operator_param));
          } else if constexpr (std::is_same_v<ParamType, LsTwoOptParam>) {
            operators.emplace_back(std::make_unique<LsTwoOptOperator>(operator_param));
          }
        },
        param);
  }
  return operators;
}

LsReport run_local_search(Solution* sol, const LsParameter& parameter,
                          const std::vector<LsOperatorParam>& operator_params,
                          const SolverContext* context,
                          const OrderRelationshipGraph* order_relationship_graph) {
  const LsSearch search(context, parameter, operator_params, order_relationship_graph);
  return search.run(sol);
}
