/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#include "hc_vehicle.h"

// ====== implement of HcVehicleCapacity ======
std::unique_ptr<HardConstrScore> HcVehicleCapacity::eval(Load *load)
{
    const std::vector<long> &dim_vals = load->vehicle->vehicle_model->dim_vals;
    const std::vector<long> &peak_load_dims = load->get_peak_load_dims();

    int dim_len = dim_vals.size();
    for (int u = 0; u < dim_len; u++)
    {
        if (peak_load_dims[u] > dim_vals[u])
        {
            return std::make_unique<HardConstrScore>(
                this->code, false, 1, 1,
                InfeasibleReasonCollection::VEHICLE_CAPACITY
            );
        }
    }
    return std::make_unique<HardConstrScore>(this->code);
}
