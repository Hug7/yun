/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#include "pr_vrp.h"


Load *ProblemVRP::construct_load_by_order(std::vector<Order *> &orders)
{
    Load *load = this->pd_pattern->create_load(this->scenario);
    for (auto &order : orders)
    {
        this->pd_pattern->add_order(load, order);
    }
    // update the distance and time between of node in load

    // 

    return load;
}
