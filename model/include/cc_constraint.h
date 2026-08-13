/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <string>
#include <memory>

#include "dm_score.h"
#include "dm_load.h"

/**
 * @brief the attributes of cost constraint
 */
class CostConstraint
{
public:
    /**
     * @brief unique encoding with cost constraints
     */
    const std::string code;
    /**
     * @brief priority of the constraint
     * @details priority=0 is lower priority
     */
    const int priority;
    /**
     * @brief is the constraint related to the sequence
     */
    const bool is_seq;
    /**
     * @brief is the constraint related to the vehicle
     */
    const bool is_vehicle;

    CostConstraint(const std::string code,
                   const int priority,
                   const bool is_seq,
                   const bool is_vehicle) : code(code),
                                            priority(priority),
                                            is_seq(is_seq),
                                            is_vehicle(is_vehicle) {};

    virtual ~CostConstraint() = default;

    /**
     * @brief evaluate the score of the cost constraint
     */
    virtual CostConstrScore::UPtr eval(Load *load) = 0;
};
