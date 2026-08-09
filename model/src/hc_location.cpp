/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string>

#include "dm_load.h"
#include "hc_location.h"

// ====== implement of HcMaxDropNodeCount ======
std::unique_ptr<HardConstrScore> HcMaxDropNodeCount::call(Load *load)
{
    if (load->get_drop_node_count() > this->max_drop_node_count)
    {
        return std::make_unique<HardConstrScore>(
            this->code, ReasonConstant::UNDEFINED
        );
    }
    return std::make_unique<HardConstrScore>(
            this->code, ReasonConstant::FEASIBLE
        );
}