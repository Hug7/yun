/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#include "sc_dist.h"

// ====== implement of Load CcDist ======
SoftConstrScore::UPtr ScDist::eval(Load *load)
{
    return std::make_unique<SoftConstrScore>(
        this->code,
        1,
        this->dist_factor * load->get_total_dist()
    );
}