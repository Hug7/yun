/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

/**
 * @brief 改进策略：找到改进 move 后是立刻提交，还是扫完本轮再取最优
 * @details First 一旦发现可行且改进的候选就提交并开始下一轮；Best 扫完本 pass 全部候选后
 *          只提交其中目标值最优的一个。
 */
enum class LsImprovement {
  First,
  Best,
};

/**
 * @brief 邻域搜索参数
 * @details 全量搜索：每个 pass 枚举并评估所有候选，按 `improvement` 决定提交哪个 move，
 *          直到一整个 pass 都找不到改进为止；不设时间预算、迭代次数或候选上限。
 */
struct LsParameter {
  /**
   * @brief 严格改进阈值
   */
  double epsilon{1e-9};
  /**
   * @brief 改进策略
   */
  LsImprovement improvement{LsImprovement::Best};
  /**
   * @brief 评估次数上限，0 表示不限
   * @details 达到上限时不再评估新候选，本 pass 已找到的最优 move 照常提交，然后结束搜索
   */
  int max_evaluated{0};
  /**
   * @brief KNN 剪枝用的近邻数，0 表示不做 KNN 剪枝
   * @details 搜索器按该值构建订单 K 近邻图并持有复用；k 越小剪得越狠
   */
  int knn_neighbor_count{10};
};
