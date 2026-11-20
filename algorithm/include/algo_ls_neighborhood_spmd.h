/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <vector>

#include "algo_ls_neighborhood.h"

class Load;
class Solution;
class PickDropPattern;

/**
 * @brief SPMD 的 LsNeighborhood 适配器
 * @details 单元 = PICK 节点之后的 DROP 节点；结构编辑全部委托给 `LsNodeEdit` 的
 *          node/activity 原语，只作用于调用方传入的 Load 副本。
 */
class SpmdNeighborhood : public LsNeighborhood {
 public:
  /**
   * @brief 构造适配器
   * @param pattern SPMD 的 PickDropPattern，仅用于 `copy_load` 的深拷贝；不允许为空
   */
  explicit SpmdNeighborhood(PickDropPattern* pattern);

  [[nodiscard]] int movable_unit_count(const Solution& sol, int load_ind) const override;

  [[nodiscard]] std::vector<std::vector<int>> load_unit_orders(const Solution& sol,
                                                               int load_ind) const override;

  [[nodiscard]] std::vector<int> candidate_target_loads(const Solution& sol,
                                                        const LsBlock& moved) const override;

  [[nodiscard]] std::vector<int> target_slots(const Solution& sol, const LsBlock& moved,
                                              int to_load) const override;

  [[nodiscard]] Load* copy_load(const Load& load) const override;

  bool apply(Load* from, Load* to, const LsMove& move) const override;

  void canonicalize(Load* load) const override;

 private:
  PickDropPattern* pattern_{nullptr};
};
