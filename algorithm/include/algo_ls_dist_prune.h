/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <cstdint>
#include <optional>
#include <vector>

#include "algo_ls_move.h"

class DistMatrix;
class Solution;

/**
 * @brief 基于里程增量的候选剪枝
 * @details 只读当前解，按 move 生效后受影响车次的里程变化判断候选：里程增加的直接丢弃。
 *          不做结构编辑、不评估约束。涉及的车次当前不可行时一律不剪。
 */
namespace LsDistPrune {

/**
 * @brief 一辆车次的路线快照
 * @details 链形固定为 START -> PICK -> k 个 DROP -> END；只记站点下标，不持有节点指针。
 *          表对应某一次解的形态，解被提交修改后必须重建。
 */
struct RouteSnapshot {
  /**
   * @brief 该车次车型的距离矩阵
   */
  const DistMatrix* dist_matrix{nullptr};
  /**
   * @brief PICK 节点与 END 节点的站点下标
   */
  int pick_loc_ind{-1};
  int end_loc_ind{-1};
  /**
   * @brief 各 DROP 单元的站点下标，按路线顺序
   */
  std::vector<int> unit_loc_inds;
  /**
   * @brief 该车次当前是否可行
   */
  bool feasible{true};

  [[nodiscard]] int unit_count() const { return static_cast<int>(this->unit_loc_inds.size()); }
};

/**
 * @brief 全解的路线快照集合，按 Solution::loads 的顺序索引
 */
class DistanceTable {
 public:
  DistanceTable() = default;

  explicit DistanceTable(std::vector<RouteSnapshot> routes);

  /**
   * @brief 从当前解构建全部车次的快照
   */
  static DistanceTable from_solution(const Solution& sol);

  [[nodiscard]] bool empty() const { return this->routes_.empty(); }

  [[nodiscard]] int load_count() const { return static_cast<int>(this->routes_.size()); }

  /**
   * @brief 取某个车次的快照；越界返回 nullptr
   */
  [[nodiscard]] const RouteSnapshot* route(int load_ind) const;

 private:
  std::vector<RouteSnapshot> routes_;
};

/**
 * @brief move 生效后受影响车次的里程变化
 * @return move 越界、缺距离矩阵等无法判定时返回 std::nullopt
 */
[[nodiscard]] std::optional<std::int64_t> distance_delta(const LsMove& move,
                                                         const DistanceTable& table);

/**
 * @brief 是否保留候选：里程不增即保留
 * @details 涉及的车次当前不可行、或里程无法判定时也保留，交给评估阶段处理
 */
[[nodiscard]] bool passes(const LsMove& move, const DistanceTable& table);

}  // namespace LsDistPrune
