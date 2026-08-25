/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <vector>

#include "bd_common.h"
#include "bd_vehicle.h"

class AvailableVehicle {
 public:
  /**
   * @brief 可用车辆集合
   */
  std::vector<Vehicle*> vehicles;
  /**
   * @brief 可用车辆集合索引
   */
  Bitset::UPtr vehicle_bitset;

  AvailableVehicle(std::vector<Vehicle*>& vehicles, Bitset::UPtr vehicle_bitset)
      : vehicles(vehicles), vehicle_bitset(std::move(vehicle_bitset)) {}

  ~AvailableVehicle();

  /**
   * @brief 添加车辆
   * @param vehicle 车辆
   */
  void add_vehicle(Vehicle* vehicle);
};
