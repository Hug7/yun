/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <vector>

#include "algo_graph_knn.h"
#include "algo_ls_move.h"

class LsNeighborhood;
class Solution;

/**
 * @brief 基于订单 K 近邻图的候选剪枝
 * @details 只判断「这个 move 新建的相邻关系里有没有 KNN 边」，不做结构编辑、不评估目标值；
 *          由引擎在评估前调用，命中不了的候选直接丢弃。
 */
namespace LsKnnPrune {

/**
 * @brief 「单元 → 订单」表的快照
 * @details units[load_ind][unit_ind] = 该单元承载的订单下标；表对应某一次解的形态，
 *          解被提交修改后必须重建。
 */
class UnitOrderTable {
 public:
  UnitOrderTable() = default;

  explicit UnitOrderTable(std::vector<std::vector<std::vector<int>>> units);

  /**
   * @brief 从当前解构建全部 Load 的「单元 → 订单」表
   */
  [[nodiscard]] static UnitOrderTable from_solution(const Solution& sol,
                                                    const LsNeighborhood& neighborhood);

  /**
   * @brief 该 Load 的单元数；下标越界返回 0
   */
  [[nodiscard]] int unit_count(int load_ind) const;

  /**
   * @brief 该单元的订单下标；下标越界返回 nullptr
   */
  [[nodiscard]] const std::vector<int>* unit_orders(int load_ind, int unit_ind) const;

 private:
  std::vector<std::vector<std::vector<int>>> units_;
};

/**
 * @brief 两个订单是否互为 KNN 近邻
 * @details 图中任一方向的边都算，避免只依赖有向的 k 近邻排名
 */
[[nodiscard]] bool is_knn_edge(const KnnGraph& graph, int from_order, int to_order);

/**
 * @brief 候选是否通过剪枝
 * @details 判据：move 新建的相邻单元对里，至少有一对（其订单）构成 KNN 边。新建相邻对：
 *          relocate → (目标槽位前驱, 块首单元)、(块尾单元, 目标槽位后继)；
 *          exchange → 两个块各自左/右邻居与新落位块之间；
 *          reverse  → (段前单元, 段尾单元)、(段首单元, 段后单元)。
 *          同车 relocate 的槽位按移除源块后的序列计，与 `target_slots` 一致。
 *          取不到任何相邻对时（例如只有一个单元的车次）保留候选，避免过度剪枝。
 */
[[nodiscard]] bool passes(const LsMove& move, const UnitOrderTable& table, const KnnGraph& graph);

}  // namespace LsKnnPrune
