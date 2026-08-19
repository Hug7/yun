/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#include "hc_label.h"

// ====== implement of HcOrderLabelValueMaxCount ======
HardConstrScore::UPtr HcOrderLabelValueMaxCount::eval(Load* load) {
  auto order_labelset_value_bitset = load->get_order_labelset_value_bitset();
  for (auto& val : this->distinct_label_value_max_counts) {
    if (order_labelset_value_bitset->bitsets[val.first]->popcount() > val.second) {
      return std::make_unique<HardConstrScore>(this->code, false, 1, 1, this->seq_type,
                                               this->vehicle_type,
                                               InfeasibleReasonCollection::AVAILABLE_VEHICLE);
    }
  }
  return std::make_unique<HardConstrScore>(this->code);
}
