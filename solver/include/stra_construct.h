/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <limits>

#include "pdm_workspace.h"

/**
 * @brief knn参数
 */
struct KnnParameter {
  /**
   * @brief 近邻数量
   */
  int neighbor_count{8};

  KnnParameter() = default;

  explicit KnnParameter(const int neighbor_count) : neighbor_count(neighbor_count) {}
};

/**
 * @brief 构造启发式
 */
namespace ConstructHeuristic {
/**
 * @brief knn 贪心插入的候选：把 order 插到 load 末尾的指标(rank)和实体索引
 */
struct KnnCandidate {
  /**
   * @brief knn 图中的 rank，越小越优先
   */
  int rank;
  /**
   * @brief 订单索引
   */
  int order_ind;
  /**
   * @brief 车次索引
   */
  int load_ind;
};

/**
 * @brief 候选 a 是否严格优于候选 b
 * @details 判断规则：rank 相同时先比 order_ind、再比 load_ind。
 */
[[nodiscard]] constexpr bool knn_candidate_better(const KnnCandidate& a, const KnnCandidate& b) {
  if (a.rank != b.rank) {
    return a.rank < b.rank;
  }
  if (a.order_ind != b.order_ind) {
    return a.order_ind < b.order_ind;
  }
  return a.load_ind < b.load_ind;
}

/**
 * @brief 增量维护最优 knn 候选
 * @details 结果与 offer 的顺序无关（由 knn_candidate_better 保证幂等性）；
 */
class KnnCandidatePicker {
 public:
  /**
   * @brief 提供一个候选，比当前最优更优时替换
   */
  void offer(const KnnCandidate& candidate) {
    if (!has_value_ || knn_candidate_better(candidate, best_)) {
      best_ = candidate;
      has_value_ = true;
    }
  }

  /**
   * @brief 是否已经收到过候选
   */
  [[nodiscard]] bool has_value() const { return has_value_; }

  /**
   * @brief 当前最优候选，调用前需 has_value() 为真
   */
  [[nodiscard]] const KnnCandidate& best() const { return best_; }

  /**
   * @brief 当前最优候选的 rank；没有候选时为最大值
   */
  [[nodiscard]] int best_rank() const {
    return has_value_ ? best_.rank : std::numeric_limits<int>::max();
  }

  /**
   * @brief 该 rank 的候选是否已不可能更优（可跳过后续试插）
   * @details 用于剪枝，只有 rank 严格更大的候选才允许跳过
   */
  [[nodiscard]] bool worse_than_best(const int rank) const { return rank > best_rank(); }

 private:
  /**
   * @brief 最优候选
   */
  KnnCandidate best_{};

  /**
   * @brief 是否找到候选解
   */
  bool has_value_{false};
};

/**
 * @brief K近邻构造解
 * @param workspace 工作空间
 * @param parameter 构造参数
 */
void k_nearest_neighbor(Workspace* workspace, const KnnParameter& parameter);
}  // namespace ConstructHeuristic
