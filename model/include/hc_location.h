/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <string>

#include "dm_load.h"
#include "hc_constraint.h"

/**
 * @brief hard constraint for max drop node count
 */
class HcMaxDropNodeCount : public HardConstraint<Load>
{
public:
    const int max_drop_node_count;

    HcMaxDropNodeCount(const int max_drop_node_count)
        : HardConstraint("MaxDropNodeCount", 0, false, false),
          max_drop_node_count(max_drop_node_count) {};

    /**
     * @brief calculate the score of the hard constraint
     */
    std::unique_ptr<HardConstrScore> call(Load *t) override;
};
