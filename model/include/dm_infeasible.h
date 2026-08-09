/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <string>

/**
 * @brief infeasible of reason
 */
class InfeasibleReason
{
public:
    /**
     * @brief infeasible reason code
     */
    const std::string code;
    /**
     * @brief constraint code
     */
    const std::string constr_code;
    /**
     * @brief infeasible reason message
     */
    const std::string msg;

    InfeasibleReason(const std::string &code,
                     const std::string &constr_code,
                     const std::string &msg) : code(code),
                                              constr_code(constr_code),
                                              msg(msg) {}

    bool operator==(const InfeasibleReason &other) const noexcept
    {
        return code == other.code;
    }

    bool operator!=(const InfeasibleReason &other) const noexcept
    {
        return !(*this == other);
    }
};

// ---- predefined infeasible reason constant ----
namespace ReasonConstant
{
    inline const InfeasibleReason FEASIBLE{"1000", "ALL", "the constraint is feasible"};

    inline const InfeasibleReason UNDEFINED{"1001", "ALL", "undefined infeasible reason"};

    inline const InfeasibleReason VIOLATE_MAX_DROP_NODE_COUNT{"1002", "ALL", "undefined infeasible reason"};
}
