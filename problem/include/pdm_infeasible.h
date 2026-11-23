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

inline const InfeasibleReason FILO_LOAD_UNLOAD_POLICY{
    "1007", "HcLoadUnloadPolicyFILO", "订单提卸货顺序不满足先入后出(FILO)",
    "the order pickup/drop sequence violates the FILO policy"};

inline const InfeasibleReason FIFO_LOAD_UNLOAD_POLICY{
    "1008", "HcLoadUnloadPolicyFIFO", "订单提卸货顺序不满足先入先出(FIFO)",
    "the order pickup/drop sequence violates the FIFO policy"};

inline const InfeasibleReason TIME_WINDOW{"1009", "HcTimeWindow", "不满足时间窗约束",
                                          "not meet the time window constraint"};

inline const InfeasibleReason NO_CHARGE_RATE{"1010", "CcCharge", "未匹配到计费费率",
                                             "no charge rate matched"};

inline const InfeasibleReason ORDER_LABEL_VALUE_EXCLUSIVE{
    "1011", "HcLabelValueExclusive", "车次包含互斥的订单标签值",
    "the load contains mutually exclusive order label values"};

inline const InfeasibleReason PICK_LOCATION_LABEL_VALUE_EXCLUSIVE{
    "1012", "HcLabelValueExclusive", "车次包含互斥的提货站点标签值",
    "the load contains mutually exclusive pick location label values"};

inline const InfeasibleReason DROP_LOCATION_LABEL_VALUE_EXCLUSIVE{
    "1013", "HcLabelValueExclusive", "车次包含互斥的卸货站点标签值",
    "the load contains mutually exclusive drop location label values"};

inline const InfeasibleReason ORDER_LABEL_VALUE_COMPATIBLE{
    "1014", "HcLabelValueCompatible", "车次包含不可拼的订单标签值",
    "the load contains incompatible order label values"};

inline const InfeasibleReason PICK_LOCATION_LABEL_VALUE_COMPATIBLE{
    "1015", "HcLabelValueCompatible", "车次包含不可拼的提货站点标签值",
    "the load contains incompatible location label values"};

inline const InfeasibleReason DROP_LOCATION_LABEL_VALUE_COMPATIBLE{
    "1016", "HcLabelValueCompatible", "车次包含不可拼的卸货站点标签值",
    "the load contains incompatible location label values"};

inline const InfeasibleReason ORDER_LABEL_VALUE_MAX_COUNT{
    "1017", "HcLabelValueMaxCount", "车次的订单标签值种类数超过上限",
    "the distinct order label value count exceeds the maximum for load"};

inline const InfeasibleReason PICK_LOCATION_LABEL_VALUE_MAX_COUNT{
    "1018", "HcLabelValueMaxCount", "车次的提货站点标签值种类数超过上限",
    "the distinct pick location label value count exceeds the maximum for load"};

inline const InfeasibleReason DROP_LOCATION_LABEL_VALUE_MAX_COUNT{
    "1019", "HcLabelValueMaxCount", "车次的卸货站点标签值种类数超过上限",
    "the distinct drop location label value count exceeds the maximum for load"};

inline const InfeasibleReason LOCATION_LABEL_VALUE_MAX_COUNT{
    "1020", "HcLabelValueMaxCount", "车次的提卸货站点标签值种类数超过上限",
    "the distinct pick/drop location label value count exceeds the maximum for load"};

}  // namespace InfeasibleReasonCollection
