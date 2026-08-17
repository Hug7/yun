/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <string>

/**
 * @brief infeasible of reason
 */
class InfeasibleReason {
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
   * @brief infeasible reason message cn
   */
  const std::string msg_cn;
  /**
   * @brief infeasible reason message en
   */
  const std::string msg_en;

  InfeasibleReason(const std::string& code, const std::string& constr_code,
                   const std::string& msg_cn, const std::string& msg_en)
      : code(code), constr_code(constr_code), msg_cn(msg_cn), msg_en(msg_en) {}

  bool operator==(const InfeasibleReason& other) const noexcept { return code == other.code; }

  bool operator!=(const InfeasibleReason& other) const noexcept { return !(*this == other); }

  
};

// ---- predefined infeasible reason constant ----
namespace InfeasibleReasonCollection {
inline const InfeasibleReason FEASIBLE{"1000", "ALL", "有解", "the constraint is feasible"};

inline const InfeasibleReason UNDEFINED{"1001", "ALL", "未知无解原因",
                                        "undefined infeasible reason"};

inline const InfeasibleReason MAX_PICK_NODE_COUNT{
    "1002", "MaxPickNodeCount", "超过车次的最大提货节点数", "out of max pick node count in load"};

inline const InfeasibleReason MAX_DROP_NODE_COUNT{
    "1003", "MaxDropNodeCount", "超过车次的最大卸货节点数", "out of max drop node count in load"};

inline const InfeasibleReason VEHICLE_CAPACITY{"1004", "HcVehicleCapacity", "超过车次的最大装载",
                                               "out of vehicle capacity in load"};
}  // namespace InfeasibleReasonCollection
