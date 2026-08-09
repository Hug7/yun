/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "c_constant.h"
#include "bd_dist_matrix.h"

/**
 * @brief parameter configuration
 */
class Parameter
{
public:
    /**
     * @brief the type of pick and drop pattern
     */
    PickDropPatternType pick_drop_pattern{PickDropPatternType::SPMD};
    /**
     * @brief default dist matrix
     * @details if the dist matrix is not specified, use this one
     */
    DistMatrix *default_dist_matrix{nullptr};

    Parameter() {};
};