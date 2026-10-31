/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#include "hc_vehicle.h"

// ====== implement of HcVehicleCapacity ======
HardConstrScore::UPtr HcVehicleCapacity::eval(Load* load) {
  const std::vector<std::int64_t>& dim_vals = load->vehicle->vehicle_model->dim_vals;
  const std::vector<std::int64_t>& peak_load_dims = load->get_peak_load_dims();

  const size_t dim_len = dim_vals.size();
  for (size_t u = 0; u < dim_len; u++) {
    if (peak_load_dims[u] > dim_vals[u]) {
      return std::make_unique<HardConstrScore>(this->code, false, 1, 1, this->seq_type,
                                               this->vehicle_type,
                                               InfeasibleReasonCollection::VEHICLE_CAPACITY);
    }
  }
  return std::make_unique<HardConstrScore>(this->code);
}
