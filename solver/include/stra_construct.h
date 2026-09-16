/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

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
 * @brief K近邻构造解
 * @param workspace 工作空间
 * @param parameter 构造参数
 */
void k_nearest_neighbor(Workspace* workspace, const KnnParameter& parameter);
}  // namespace ConstructHeuristic
