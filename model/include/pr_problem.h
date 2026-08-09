/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <vector>

#include "se_scenario.h"
#include "dm_order.h"
#include "dm_load.h"
#include "pr_pattern.h"

class Problem
{
public:
    const Scenario *scenario;

    PickDropPattern *pd_pattern;

    Problem(const Scenario *scenario);

    /**
     * @brief construct a new Load object for best vehicle
     */
    virtual Load *construct_load_by_order(std::vector<Order *> &orders) = 0;
    /**
     * @brief construct a new Load object for a vehicle
     */
    virtual Load *construct_load_by_order(std::vector<Order *> &orders, Vehicle *vehicle) = 0;
};
