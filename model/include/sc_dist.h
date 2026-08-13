/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <string>

#include "sc_constraint.h"

/**
 * @brief the attributes of soft constraint
 */
class ScDist : public SoftConstraint
{
public:
    /**
     * distance factor
     */
    const double dist_factor;

    ScDist(const double dist_factor)
        : SoftConstraint("ScDist", 0, true, false), dist_factor(dist_factor) {}
    /**
     * @brief evaluate the score of the soft constraint
     */
    SoftConstrScore::UPtr eval(Load *load) override;
};
