/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <string>
#include <vector>

#include "algo_ls_operator.h"

/**
 * @brief 单车内反转一段连续单元（2-opt）的参数
 */
struct LsTwoOptParam {
  int min_span_len{2};
  /**
   * @brief 0 = 不限
   */
  int max_span_len{0};
};

/**
 * @brief 2-opt 算子
 */
class LsTwoOptOperator final : public LsOperator {
 public:
  /**
   * @brief 算子参数
   */
  const LsTwoOptParam parameter;

  explicit LsTwoOptOperator(const LsTwoOptParam& _parameter);

  void enumerate(const LsOperatorContext& context, std::vector<LsMove>& out) const override;

  [[nodiscard]] std::string format_parameters() const override;
};
