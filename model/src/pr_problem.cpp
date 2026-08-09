/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#include "pr_problem.h"

Problem::Problem(const Scenario *scenario) : scenario(scenario)
{
    switch (scenario->parameter->pick_drop_pattern)
    {
    case PickDropPatternType::SPMD:
        this->pd_pattern = new SPMD(scenario);
        break;
    case PickDropPatternType::MPMD:
        throw std::invalid_argument("unsupported pick drop pattern!");
        break;
    case PickDropPatternType::PDP:
        throw std::invalid_argument("unsupported pick drop pattern!");
        break;
    default:
        throw std::invalid_argument("invalid pick drop pattern!");
        break;
    }
}
