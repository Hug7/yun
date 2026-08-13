/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string>

#include "dm_load.h"
#include "hc_location.h"

// ====== implement of HcMaxPickNodeCount ======
HardConstrScore::UPtr HcMaxPickNodeCount::eval(Load *load)
{
    if (load->get_pick_node_count() > this->max_pick_node_count)
    {
        return std::make_unique<HardConstrScore>(
            this->code, false, 1, 1,
            InfeasibleReasonCollection::MAX_PICK_NODE_COUNT
        );
    }
    return std::make_unique<HardConstrScore>(this->code);
}

// ====== implement of HcMaxDropNodeCount ======
HardConstrScore::UPtr HcMaxDropNodeCount::eval(Load *load)
{
    if (load->get_drop_node_count() > this->max_drop_node_count)
    {
        return std::make_unique<HardConstrScore>(
            this->code, false, 1, 1,
            InfeasibleReasonCollection::MAX_DROP_NODE_COUNT
        );
    }
    return std::make_unique<HardConstrScore>(this->code);
}
