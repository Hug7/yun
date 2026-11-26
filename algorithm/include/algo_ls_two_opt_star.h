/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <string>
#include <vector>

#include "algo_ls_operator.h"

/**
 * @brief 跨线路尾部交换订单段（2-opt*）的参数
 * @details 两个车次各取一段尾部连续单元互换：新 A = prefix(A) + tail(B)，
 *          新 B = prefix(B) + tail(A)。段长默认 2 起，避免与只交换单单元的
 *          `LsInterRouteExchange` 产生同质候选。
 */
struct LsTwoOptStarParam {
  /**
   * @brief 最短尾部段长，必须 >= 1
   */
  int min_tail_len{2};
  /**
   * @brief 最长尾部段长；0 = 不限（不超过该车次的单元数）
   */
  int max_tail_len{0};
};

/**
 * @brief 2-opt* 算子
 * @details 只枚举跨车次（同一 PICK 站点）的尾部段；候选复用 `LsExchangeMove`，
 *          因为两个块都是后缀时「各自占据对方原位」的语义就是尾部交换。
 */
class LsTwoOptStarOperator final : public LsOperator {
 public:
  /**
   * @brief 算子参数
   */
  const LsTwoOptStarParam parameter;

  explicit LsTwoOptStarOperator(const LsTwoOptStarParam& _parameter);

  void enumerate(const LsOperatorContext& context, std::vector<LsMove>& out) const override;

  [[nodiscard]] std::string format_parameters() const override;
};
