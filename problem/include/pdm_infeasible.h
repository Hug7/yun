/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <string>

/**
 * @brief 不可解原因
 */
class InfeasibleReason {
 public:
  /**
   * @brief 不可解原因编码
   */
  const std::string code;
  /**
   * @brief 约束编码
   */
  const std::string constr_code;
  /**
   * @brief 中文不可解原因
   */
  const std::string msg_cn;
  /**
   * @brief 英文不可解原因
   */
  const std::string msg_en;

  InfeasibleReason(std::string code, std::string constr_code, std::string msg_cn,
                   std::string msg_en)
      : code(std::move(code)),
        constr_code(std::move(constr_code)),
        msg_cn(std::move(msg_cn)),
        msg_en(std::move(msg_en)) {}

  bool operator==(const InfeasibleReason& other) const noexcept { return code == other.code; }

  bool operator!=(const InfeasibleReason& other) const noexcept { return !(*this == other); }
};

// ---- predefined infeasible reason constant ----
namespace InfeasibleReasonCollection {
inline const InfeasibleReason FEASIBLE{"1000", "ALL", "有解", "the constraint is feasible"};

inline const InfeasibleReason UNDEFINED{"1001", "ALL", "未知无解原因",
                                        "undefined infeasible reason"};

inline const InfeasibleReason VEHICLE_RESOURCE{"1002", "ALL", "车辆资源不足",
                                              "the vehicle resources are insufficient"};

inline const InfeasibleReason MAX_PICK_NODE_COUNT{
    "1003", "MaxPickNodeCount", "超过车次的最大提货节点数", "out of max pick node count in load"};

inline const InfeasibleReason MAX_DROP_NODE_COUNT{
    "1004", "MaxDropNodeCount", "超过车次的最大卸货节点数", "out of max drop node count in load"};

inline const InfeasibleReason VEHICLE_CAPACITY{"1005", "HcVehicleCapacity", "超过车次的最大装载",
                                               "out of vehicle capacity in load"};

inline const InfeasibleReason AVAILABLE_VEHICLE{"1006", "HcAvailableVehicle", "车辆不可用",
                                                "the vehicle is unavailable for load"};

inline const InfeasibleReason DISTINCT_LABEL_VALUE_MAX_COUNT{
    "1007", "HcOrderLabelValueMaxCount", "订单的标签值超过最大数量",
    "the distinct label value of the order exceeds the maximum quantity for load"};

inline const InfeasibleReason FILO_LOAD_UNLOAD_POLICY{
    "1008", "HcLoadUnloadPolicyFILO", "订单提卸货顺序不满足先入后出(FILO)",
    "the order pickup/drop sequence violates the FILO policy"};

inline const InfeasibleReason FIFO_LOAD_UNLOAD_POLICY{
    "1009", "HcLoadUnloadPolicyFIFO", "订单提卸货顺序不满足先入先出(FIFO)",
    "the order pickup/drop sequence violates the FIFO policy"};

inline const InfeasibleReason TIME_WINDOW{"1010", "HcTimeWindow", "不满足时间窗约束",
                                          "not meet the time window constraint"};

}  // namespace InfeasibleReasonCollection
