/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#include "algo_ls_search.h"

#include <chrono>
#include <cmath>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

#include "algo_ls_dist_prune.h"
#include "algo_ls_knn_prune.h"
#include "algo_ls_neighborhood.h"
#include "algo_ls_operator.h"
#include "pdm_context.h"
#include "pdm_load.h"
#include "pdm_solution.h"
#include "prob_problem.h"

namespace {

/**
 * @brief 收集一个 move 影响的 Load 下标（去重，最多 2 个）
 * @details relocate → {block.load, to_load}；exchange → {first.load, second.load}；
 *          reverse → {block.load}
 */
void collect_touched_loads(const LsMove& move, int (&load_inds)[2], int& load_count) {
  load_count = 0;
  const auto add = [&load_inds, &load_count](const int load_ind) {
    if (load_ind < 0) {
      return;
    }
    for (int u = 0; u < load_count; ++u) {
      if (load_inds[u] == load_ind) {
        return;
      }
    }
    if (load_count < 2) {
      load_inds[load_count] = load_ind;
      ++load_count;
    }
  };
  std::visit(
      [&add](const auto& typed_move) {
        using MoveType = std::decay_t<decltype(typed_move)>;
        if constexpr (std::is_same_v<MoveType, LsRelocateMove>) {
          add(typed_move.block.load_ind);
          add(typed_move.to_load);
        } else if constexpr (std::is_same_v<MoveType, LsExchangeMove>) {
          add(typed_move.first.load_ind);
          add(typed_move.second.load_ind);
        } else if constexpr (std::is_same_v<MoveType, LsReverseMove>) {
          add(typed_move.block.load_ind);
        }
      },
      move);
}

/**
 * @brief 候选评估的计划：受影响的 Load 副本与目标值增量
 */
struct LsPlan {
  int load_count{0};
  int load_indices[2]{-1, -1};
  std::unique_ptr<Load> copies[2];
  double delta{0};
};

}  // namespace

// ====== implement of LsSearch::Impl ======
struct LsSearch::Impl {
  const SolverContext* context;
  LsParameter parameter;
  const std::vector<LsOperatorParam>& operator_params;
  const OrderRelationshipGraph* order_relationship_graph;
  std::unique_ptr<LsNeighborhood> neighborhood;
  std::chrono::steady_clock::time_point started_at;
  ///< KNN 剪枝图：首次 run 时构建一次，之后复用
  std::optional<KnnGraph> knn_graph;

  Impl(const SolverContext* _context, const LsParameter& _parameter,
       const std::vector<LsOperatorParam>& _operator_params,
       const OrderRelationshipGraph* _order_relationship_graph)
      : context(_context),
        parameter(_parameter),
        operator_params(_operator_params),
        order_relationship_graph(_order_relationship_graph),
        neighborhood(create_ls_neighborhood(_context)),
        started_at(std::chrono::steady_clock::now()) {
    if (this->neighborhood == nullptr) {
      throw std::invalid_argument(
          "LsSearch: current pick_drop_pattern does not support local search");
    }
    if (!std::isfinite(this->parameter.epsilon) || this->parameter.epsilon < 0) {
      throw std::invalid_argument("LsSearch: `epsilon` must be a finite value >= 0");
    }
    if (this->parameter.max_evaluated < 0) {
      throw std::invalid_argument("LsSearch: `max_evaluated` must be >= 0");
    }
    if (this->parameter.knn_neighbor_count < 0) {
      throw std::invalid_argument("LsSearch: `knn_neighbor_count` must be >= 0");
    }
  }

  /**
   * @brief 在副本上执行 move 并做全量重评
   * @return true 表示结构合法且可行；delta 有效（可能 >= 0）
   */
  [[nodiscard]] bool evaluate(Solution* sol, const LsMove& move, LsPlan* plan) const {
    int load_indices[2]{-1, -1};
    int load_count = 0;
    collect_touched_loads(move, load_indices, load_count);
    if (load_count < 1 || load_count > 2) {
      return false;
    }
    int load_len = static_cast<int>(sol->loads.size());
    for (int u = 0; u < load_count; ++u) {
      if (load_indices[u] < 0 || load_indices[u] >= load_len ||
          sol->loads[load_indices[u]] == nullptr) {
        return false;
      }
      const Load* live_load = sol->loads[load_indices[u]];
      if (live_load->get_order_count() == 0) {
        return false;
      }
    }

    double old_obj_val = 0;
    for (int u = 0; u < load_count; ++u) {
      old_obj_val += sol->loads[load_indices[u]]->get_obj_val();
    }

    std::unique_ptr<Load> copies[2];
    for (int u = 0; u < load_count; ++u) {
      copies[u].reset(this->neighborhood->copy_load(*sol->loads[load_indices[u]]));
      if (copies[u] == nullptr) {
        return false;
      }
      copies[u]->reset_route_profile();
    }

    const bool applied = std::visit(
        [this, &copies, load_count](const auto& typed_move) {
          using MoveType = std::decay_t<decltype(typed_move)>;
          if (load_count == 1) {
            return this->neighborhood->apply(copies[0].get(), copies[0].get(), typed_move);
          }
          if constexpr (std::is_same_v<MoveType, LsRelocateMove> ||
                        std::is_same_v<MoveType, LsExchangeMove>) {
            return this->neighborhood->apply(copies[0].get(), copies[1].get(), typed_move);
          }
          return false;
        },
        move);
    if (!applied) {
      return false;
    }

    for (int u = 0; u < load_count; ++u) {
      this->neighborhood->canonicalize(copies[u].get());
      if (copies[u]->get_order_count() == 0) {
        // 不允许把车次搬空
        return false;
      }
    }

    double new_obj_val = 0;
    for (int u = 0; u < load_count; ++u) {
      if (copies[u]->store_buffer != nullptr) {
        throw std::logic_error("LsSearch: candidate load `store_buffer` must be nullptr");
      }
      copies[u]->reset_route_profile();
      this->context->problem->eval_load(copies[u].get());
      if (copies[u]->is_infeasible()) {
        return false;
      }
      new_obj_val += copies[u]->get_obj_val();
    }

    plan->load_count = load_count;
    plan->delta = new_obj_val - old_obj_val;
    for (int u = 0; u < load_count; ++u) {
      plan->load_indices[u] = load_indices[u];
      plan->copies[u] = std::move(copies[u]);
    }
    return true;
  }

  /**
   * @brief 提交一个已评估的 plan：逐 Load 替换（车型不变，车辆资源净额为零）
   */
  void commit(Solution* sol, LsPlan* plan) const {
    for (int u = 0; u < plan->load_count; ++u) {
      if (plan->copies[u]->store_buffer != nullptr) {
        throw std::logic_error("LsSearch: candidate load `store_buffer` must be nullptr");
      }
      sol->update_load(plan->load_indices[u], plan->copies[u].release());
    }
  }
};

// ====== implement of LsSearch ======
LsSearch::LsSearch(const SolverContext* context, const LsParameter& parameter,
                   const std::vector<LsOperatorParam>& operators,
                   const OrderRelationshipGraph* order_relationship_graph)
    : impl_(std::make_unique<Impl>(context, parameter, operators, order_relationship_graph)) {}

LsSearch::~LsSearch() = default;

LsSearch::LsSearch(LsSearch&&) noexcept = default;

LsSearch& LsSearch::operator=(LsSearch&&) noexcept = default;

LsReport LsSearch::run(Solution* sol) const {
  if (sol == nullptr) {
    throw std::invalid_argument("LsSearch::run: `sol` must not be null");
  }

  std::vector<std::unique_ptr<LsOperator>> operators =
      create_ls_operators(this->impl_->operator_params);
  if (operators.empty()) {
    throw std::invalid_argument("LsSearch::run: no local search operators configured");
  }

  LsReport report;
  report.initial_obj = sol->total_obj_val();
  report.improvement =
      this->impl_->parameter.improvement == LsImprovement::First ? "first" : "best";
  report.operator_stats.resize(operators.size());
  for (int u = 0; u < static_cast<int>(operators.size()); ++u) {
    report.operator_stats[u].code = operators[u]->code;
    report.operator_stats[u].instance_index = u;
  }
  this->impl_->started_at = std::chrono::steady_clock::now();

  // KNN 剪枝图由搜索器持有：首次运行时按当前解构建一次，之后的 pass 与后续 run 复用
  const KnnGraph* knn_graph = nullptr;
  if (this->impl_->parameter.knn_neighbor_count > 0 &&
      this->impl_->order_relationship_graph != nullptr) {
    if (!this->impl_->knn_graph.has_value()) {
      this->impl_->knn_graph.emplace(construct_knn_graph(
          *this->impl_->order_relationship_graph, sol, this->impl_->parameter.knn_neighbor_count));
    }
    knn_graph = &*this->impl_->knn_graph;
  }

  int total_evaluated = 0;
  const int max_evaluated = this->impl_->parameter.max_evaluated;

  const int operator_count = static_cast<int>(operators.size());
  std::string termination_reason = "no_candidate";

  while (true) {
    ++report.passes;
    std::optional<LsPlan> best_plan;
    int best_operator_ind = -1;
    int generated_this_pass = 0;
    // 剪枝用的「单元 → 订单」快照：每个 pass 起点重建，与当前解形态保持一致
    std::optional<LsKnnPrune::UnitOrderTable> unit_order_table;
    if (knn_graph != nullptr) {
      unit_order_table.emplace(
          LsKnnPrune::UnitOrderTable::from_solution(*sol, *this->impl_->neighborhood));
    }
    // 里程剪枝用的车次快照：同样每个 pass 起点重建
    const LsDistPrune::DistanceTable distance_table =
        LsDistPrune::DistanceTable::from_solution(*sol);

    // 起点算子按 pass 轮转：first 模式下避免总由同一算子先拿到改进
    const int start_operator = (report.passes - 1) % operator_count;
    const bool first_improvement = this->impl_->parameter.improvement == LsImprovement::First;
    bool improved_this_pass = false;
    bool budget_exhausted = false;

    for (int k = 0; k < operator_count && !improved_this_pass && !budget_exhausted; ++k) {
      const int operator_ind = (start_operator + k) % operator_count;
      const LsOperatorContext op_context{.solution = sol,
                                         .neighborhood = this->impl_->neighborhood.get()};
      std::vector<LsMove> moves;
      operators[operator_ind]->enumerate(op_context, moves);
      LsOperatorStat& op_stat = report.operator_stats[operator_ind];
      op_stat.generated += static_cast<int>(moves.size());
      report.generated += static_cast<int>(moves.size());
      generated_this_pass += static_cast<int>(moves.size());

      for (const LsMove& move : moves) {
        if (max_evaluated > 0 && total_evaluated >= max_evaluated) {
          budget_exhausted = true;
          break;
        }
        // 会让涉事车次里程增加的候选在评估前剪掉
        if (!LsDistPrune::passes(move, distance_table)) {
          ++op_stat.dist_pruned;
          ++report.dist_pruned;
          continue;
        }
        // 新建相邻关系里没有 KNN 边的候选在评估前剪掉
        if (unit_order_table.has_value() &&
            !LsKnnPrune::passes(move, *unit_order_table, *knn_graph)) {
          ++op_stat.pruned;
          ++report.pruned;
          continue;
        }
        ++op_stat.evaluated;
        ++report.evaluated;
        ++total_evaluated;
        LsPlan plan;
        bool feasible = false;
        try {
          feasible = this->impl_->evaluate(sol, move, &plan);
        } catch (const std::logic_error&) {
          throw;
        } catch (const std::exception& e) {
          this->impl_->context->logger->warn("local search candidate failed: {}", e.what());
          continue;
        }
        if (!feasible) {
          continue;
        }
        ++op_stat.feasible;
        ++report.feasible;
        if (plan.delta >= -this->impl_->parameter.epsilon) {
          continue;
        }
        // first: 找到第一个改进就提交, 本 pass 立即结束
        if (first_improvement) {
          best_plan = std::move(plan);
          best_operator_ind = operator_ind;
          improved_this_pass = true;
          break;
        }
        if (!best_plan.has_value() || plan.delta < best_plan->delta) {
          best_plan = std::move(plan);
          best_operator_ind = operator_ind;
        }
      }
    }

    if (best_plan.has_value()) {
      LsOperatorStat& op_stat = report.operator_stats[best_operator_ind];
      op_stat.applied += 1;
      op_stat.delta_obj += best_plan->delta;
      ++report.applied;
      this->impl_->commit(sol, &*best_plan);
    }
    // 预算用尽: 本 pass 已找到的改进照常提交, 然后结束搜索
    if (budget_exhausted) {
      termination_reason = "max_evaluated";
      break;
    }
    if (!best_plan.has_value()) {
      termination_reason = generated_this_pass == 0 ? "no_candidate" : "no_improvement";
      break;
    }
  }

  report.final_obj = sol->total_obj_val();
  report.delta_obj = report.final_obj - report.initial_obj;
  report.termination_reason = termination_reason;
  report.elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                          std::chrono::steady_clock::now() - this->impl_->started_at)
                          .count();

  return report;
}
