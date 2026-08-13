/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "cc_constraint.h"

/**
 * @brief the attributes of cost constraint
 */
class CcDist : public CostConstraint
{
public:
    /**
     * distance factor
     */
    const double dist_factor;

    CcDist(const double dist_factor)
        : CostConstraint("CcDist", 0, true, false), dist_factor(dist_factor) {}

    /**
     * @brief evaluate the score of the cost constraint
     */
    CostConstrScore::UPtr eval(Load *load);
};
