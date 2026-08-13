/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <string>

#include "hc_constraint.h"

/**
 * @brief hard constraint for max pick node count
 */
class HcMaxPickNodeCount : public HardConstraint
{
public:
    const int max_pick_node_count;

    HcMaxPickNodeCount(const int max_pick_node_count)
        : HardConstraint("MaxPickNodeCount", 0, false, false),
          max_pick_node_count(max_pick_node_count) {};

    /**
     * @brief calculate the score of the hard constraint
     */
    HardConstrScore::UPtr eval(Load *load) override;
};

/**
 * @brief hard constraint for max drop node count
 */
class HcMaxDropNodeCount : public HardConstraint
{
public:
    const int max_drop_node_count;

    HcMaxDropNodeCount(const int max_drop_node_count)
        : HardConstraint("MaxDropNodeCount", 0, false, false),
          max_drop_node_count(max_drop_node_count) {};

    /**
     * @brief calculate the score of the hard constraint
     */
    HardConstrScore::UPtr eval(Load *load) override;
};
