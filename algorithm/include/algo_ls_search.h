/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <memory>
#include <vector>

#include "algo_ls.h"

class Solution;
class SolverContext;

/**
 * @brief 一次邻域全量搜索会话
 * @details 每个 pass 依次调用所有算子枚举并评估全部候选，提交其中最优的改进 move，
 *          直到一整个 pass 都找不到改进为止。
 */
class LsSearch {
 public:
  /**
   * @brief 构造搜索器
   * @param context 求解上下文，不允许为空
   * @param parameter 搜索参数
   * @param operators 算子列表
   * @param order_relationship_graph 订单关系图，用于构建 KNN 剪枝图；传 nullptr 表示不剪枝
   * @details KNN 图由搜索器持有：首次 `run` 时按当时解构建一次，之后的 pass 与
   *          同一搜索器上的后续 `run` 都复用该图。
   * @throws std::invalid_argument context 为空、pattern 不支持或参数非法
   */
  LsSearch(const SolverContext* context, const LsParameter& parameter,
           const std::vector<LsOperatorParam>& operators,
           const OrderRelationshipGraph* order_relationship_graph = nullptr);

  ~LsSearch();

  LsSearch(const LsSearch&) = delete;
  LsSearch& operator=(const LsSearch&) = delete;
  LsSearch(LsSearch&&) noexcept;
  LsSearch& operator=(LsSearch&&) noexcept;

  /**
   * @brief 在 sol 上原地跑一次完整搜索
   * @param sol 目标解，不允许为空，会被直接修改
   * @return 搜索报告
   */
  [[nodiscard]] LsReport run(Solution* sol) const;

 private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};
