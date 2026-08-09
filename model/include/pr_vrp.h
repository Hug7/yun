/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <vector>

#include "se_scenario.h"

#include "dm_order.h"
#include "dm_activity.h"
#include "dm_node.h"
#include "dm_load.h"
#include "pr_problem.h"

class ProblemVRP : public Problem
{
public:
    ProblemVRP(Scenario *scenario) : Problem(scenario) {};
    
    Load *construct_load_by_order(std::vector<Order *> &orders) override;
};
