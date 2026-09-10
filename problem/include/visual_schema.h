/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <array>

class PrecheckInfeasibleCargoOrderSchema {
 public:
  static constexpr char file_name[] = "PrecheckInfeasibleCargoOrder.csv";

  enum Col {
    CARGO_ORDER_CODE,
    CARRIER_CODE,
    VEHICLE_MODEL_CODE,
    INFEASIBLE_REASON_CODE,
    INFEASIBLE_REASON_CONSTRAINT_CODE,
    INFEASIBLE_REASON_MESSAGE_CN,
    INFEASIBLE_REASON_MESSAGE_EN,
  };

  static constexpr std::array headers{
      "CargoOrderCode",
      "CarrierCode",
      "VehicleModelCode",
      "InfeasibleReasonCode",
      "InfeasibleReasonConstraintCode",
      "InfeasibleReasonMessageCN",
      "InfeasibleReasonMessageEN",
  };
};
