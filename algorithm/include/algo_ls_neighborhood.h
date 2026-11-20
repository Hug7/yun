/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <memory>
#include <vector>

#include "algo_ls_move.h"

class Load;
class Solution;
class SolverContext;

/**
 * @brief pattern 中立的结构视图 + 编辑端口
 * @details 位置语义全部是「单元/槽位」索引；单元定义由适配器决定（SPMD = drop 节点）。
 *          apply / canonicalize 只作用于调用方给的 Load 副本：
 *          不改 Solution、不调 Problem::eval_load、不写共享 Cache。
 *          所有方法在主线程调用。
 */
class LsNeighborhood {
 public:
  virtual ~LsNeighborhood() = default;

  /**
   * @brief 该 Load 的可移动单元数（SPMD = drop 节点数；空 Load 为 0）
   * @details 用索引访问而不是 const Load&，让算子测试不必构造真实 Load
   */
  [[nodiscard]] virtual int movable_unit_count(const Solution& sol, int load_ind) const = 0;

  /**
   * @brief 该 Load 全部可移动单元承载的订单下标（按单元顺序）
   * @details 供候选剪枝建立「单元 → 订单」映射；单元定义与 movable_unit_count 一致
   */
  [[nodiscard]] virtual std::vector<std::vector<int>> load_unit_orders(const Solution& sol,
                                                                       int load_ind) const = 0;

  /**
   * @brief 全部可行的跨车目标 Load，按 load 下标升序；不含 moved.load_ind
   * @details SPMD 下只返回与源车共享 PICK 站点的车次
   */
  [[nodiscard]] virtual std::vector<int> candidate_target_loads(const Solution& sol,
                                                                const LsBlock& moved) const = 0;

  /**
   * @brief 目标 Load 上的全部可行插入槽位，升序
   * @details 跨车槽位按目标当前序列计；同车槽位按移除源块后的剩余序列计
   */
  [[nodiscard]] virtual std::vector<int> target_slots(const Solution& sol, const LsBlock& moved,
                                                      int to_load) const = 0;

  /**
   * @brief 深拷贝一个 Load（保留派生类型），调用方拥有返回值
   */
  [[nodiscard]] virtual Load* copy_load(const Load& load) const = 0;

  /**
   * @brief 在副本上执行结构编辑
   * @param from 源 Load 副本
   * @param to   目标 Load 副本；from == to 表示同车
   * @return false 表示结构非法，此时副本可能已部分修改，调用方必须直接丢弃副本
   */
  virtual bool apply(Load* from, Load* to, const LsMove& move) const = 0;

  /**
   * @brief 恢复 pattern 规范形（SPMD：合并相邻同站点 drop 节点）；apply 之后必须调用
   */
  virtual void canonicalize(Load* load) const = 0;
};

/**
 * @brief 按当前 pick_drop_pattern 创建适配器；不支持时返回 nullptr
 */
[[nodiscard]] std::unique_ptr<LsNeighborhood> create_ls_neighborhood(const SolverContext* context);
