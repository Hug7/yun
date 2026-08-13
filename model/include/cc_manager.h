/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <string>
#include <vector>
#include <memory>

#include "cc_constraint.h"
#include "dm_load.h"

/**
 * @brief cost constraint manager
 */
class CostConstraintManager
{
public:
    std::vector<CostConstraint *> constrs;

    int len;

    CostConstraintManager() : constrs(), len(0) {};

    ~CostConstraintManager();

    /**
     * @brief add a cost constraint to the manager
     */
    void add_constr(CostConstraint *constr);

    /**
     * @brief evaluate of all cost constraints
     */
    void eval_constrs(Load *load, LoadConstrProfile::UPtr &constr_profile);
};

