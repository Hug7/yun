/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#include "algo_alns_repair_regret_k.h"

#include <algorithm>
#include <format>
#include <limits>
#include <stdexcept>
#include <vector>

namespace {
/**
 * @brief 候选稀缺性层级
 * @details 数值越小优先级越高: 只有 1 个可行方案 > 2~K-1 个 > >=K 个
 */
enum class RegretPriority {
  OnlyOneFeasible = 0,
  Scarce = 1,
  Standard = 2,
};

/**
 * @brief Regret-K 评分
 */
struct RegretScore {
  /**
   * @brief 稀缺性层级
   */
  RegretPriority priority;
  /**
   * @brief 后悔值
   */
  double regret;
};

/**
 * @brief 计算 Regret-K 的稀缺性层级与后悔值
 * @param deltas_asc 可行插入方案的 delta, 升序(最优在前)
 * @param k 后悔值阶数, 必须 >= 2
 * @return 评分; 候选为空时返回负无穷 regret, 使其不会被选中
 */
RegretScore evaluate(const std::vector<double>& deltas_asc, const int k) {
  const int candidate_count = static_cast<int>(deltas_asc.size());
  if (candidate_count == 0) {
    return {.priority = RegretPriority::Standard,
            .regret = -std::numeric_limits<double>::infinity()};
  }

  const double best_delta = deltas_asc.front();
  // 只有一个可行方案: 最高优先级, 优先保证其能落实
  if (candidate_count == 1) {
    return {.priority = RegretPriority::OnlyOneFeasible, .regret = 0.0};
  }

  // 标准 Regret-K: sum_{i=2..min(n,k)} (c_i - c_1)
  double regret = 0.0;
  const int alternative_count = std::min(candidate_count, k);
  for (int u = 1; u < alternative_count; ++u) {
    regret += deltas_asc[u] - best_delta;
  }

  // 候选不足 K 个: 基础 Regret 之上再加稀缺性奖励, 使方案更少的 segment 优先
  if (candidate_count < k) {
    const double scarcity_unit = deltas_asc.back() - best_delta + 1.0;
    regret += static_cast<double>(k - candidate_count) * scarcity_unit;
    return {.priority = RegretPriority::Scarce, .regret = regret};
  }

  return {.priority = RegretPriority::Standard, .regret = regret};
}
}  // namespace

// ====== implement of AlnsRepairRegretK ======
AlnsRepairRegretK::AlnsRepairRegretK(const SolverContext* _context,
                                     ParallelExecutor* _parallel_executor,
                                     const AlnsRepairRegretKParam& _parameter)
    : AlnsRepairOperator(AlnsRepairOperatorCode::RepairRegretK, _context, _parallel_executor),
      parameter(_parameter) {
  if (this->parameter.regret_k < 1) {
    throw std::invalid_argument(this->code + ": `regret_k` must be >= 1, got " +
                                std::to_string(this->parameter.regret_k));
  }
}

std::string AlnsRepairRegretK::format_parameters() const {
  return std::format("regret_k={}", this->parameter.regret_k);
}

void AlnsRepairRegretK::execute(RepairMatrix* repair_matrix) {
  const size_t raw_idle_segment_count = repair_matrix->idle_segment_indices.size();
  for (size_t r = 0; r < raw_idle_segment_count; ++r) {
    {
      bool has_select = false;
      RepairCellView select_view;
      RegretPriority select_priority = RegretPriority::Standard;
      double select_regret = -std::numeric_limits<double>::infinity();
      double select_best_delta = std::numeric_limits<double>::infinity();

      for (const int segment_ind : repair_matrix->idle_segment_indices) {
        // 该 segment 行的 top-k 候选, 按 delta 升序(front() 为最优)
        const std::vector<RepairCellView> top_views =
            repair_matrix->unfold_segment_top_k(segment_ind, this->parameter.regret_k);
        if (top_views.empty()) {
          continue;
        }
        // 计算 regret
        std::vector<double> deltas_asc;
        deltas_asc.reserve(top_views.size());
        for (const auto& top_view : top_views) {
          deltas_asc.emplace_back(top_view.delta);
        }

        const RegretScore regret_score = evaluate(deltas_asc, this->parameter.regret_k);
        const double best_delta = deltas_asc.front();
        const bool better =
            !has_select || regret_score.priority < select_priority ||
            (regret_score.priority == select_priority &&
             (regret_score.regret > select_regret ||
              (regret_score.regret == select_regret && best_delta < select_best_delta)));
        if (better) {
          select_priority = regret_score.priority;
          select_regret = regret_score.regret;
          select_best_delta = best_delta;
          // top_views 升序, front() 即该 segment 的最优插入方案
          select_view = top_views.front();
          has_select = true;
        }
      }

      if (has_select) {
        // 候选与解不同构时 commit 抛 std::logic_error(内部缺陷), 不再有静默失效分支
        repair_matrix->commit(select_view);
      } else if (!repair_matrix->idle_segment_create_load()) {
        // 所有未指派 segment 都没有可行插入方案, 且无法新建车次
        break;
      }
    }
  }
}
