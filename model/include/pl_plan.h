/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <string>

#include "se_scenario.h"
#include "pr_problem.h"

class Plan
{
public:
    const Scenario *scenario;

    const Problem *problem;

};
