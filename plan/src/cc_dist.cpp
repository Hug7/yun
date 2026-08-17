/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#include "cc_dist.h"

// ====== implement of Load CcDist ======
std::unique_ptr<CostConstrScore> CcDist::eval(Load* load) {
  return std::make_unique<CostConstrScore>(this->code, 1,
                                           this->dist_factor * load->get_total_dist());
}
