/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <string>
#include <unordered_map>

#include "hc_constraint.h"

/**
 * @brief hard constraint for maximum distinct number of order label value
 */
class HcOrderLabelValueMaxCount : public HardConstraint {
 public:
  const std::unordered_map<int, int> distinct_label_value_max_counts;

  HcOrderLabelValueMaxCount(std::unordered_map<int, int>& distinct_label_value_max_counts)
      : HardConstraint("HcOrderLabelValueMaxCount", 0, false, false),
        distinct_label_value_max_counts(distinct_label_value_max_counts) {};

  /**
   * @brief calculate the score of the hard constraint
   */
  HardConstrScore::UPtr eval(Load* load) override;
};
