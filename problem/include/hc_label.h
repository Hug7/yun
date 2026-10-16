/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <unordered_map>

#include "hc_constraint.h"

/**
 * @brief 硬约束-订单标签最大种类数
 */
class HcOrderLabelValueMaxCount : public HardConstraint {
 public:
  /**
   * @brief 订单label对应最大种类数
   */
  const std::unordered_map<int, int> distinct_label_value_max_counts;

  explicit HcOrderLabelValueMaxCount(
      const std::unordered_map<int, int>& distinct_label_value_max_counts)
      : HardConstraint(HardConstrCode::OrderLabelValueMaxCount, 0, false, false),
        distinct_label_value_max_counts(distinct_label_value_max_counts) {};

  HardConstrScore::UPtr eval(Load* load) override;
};
