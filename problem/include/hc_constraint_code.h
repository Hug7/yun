/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

/**
 * @brief 硬约束编码
 */
enum class HardConstrCode {
  VehicleCapacity,
  AvailableVehicle,
  TimeWindow,
  // 需配置生效分界线
  LoadUnloadPolicyFILO,
  LoadUnloadPolicyFIFO,
  MaxPickNodeCount,
  MaxDropNodeCount,
  OrderLabelValueMaxCount,
};
