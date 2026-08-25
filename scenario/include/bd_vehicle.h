/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "bd_location.h"
#include "bd_vehicle_model.h"

class Carrier;

/**
 * @brief vehicle models with carrier
 */
class Vehicle {
 public:
  /**
   * @brief 承运商
   */
  const Carrier* carrier;

  /**
   * @brief 车辆索引
   */
  const int ind;

  /**
   * @brief 车型
   */
  const VehicleModel* vehicle_model;

  /**
   * @brief 车辆数量
   * @details 为0时代表不可用,小于0代表不限制
   */
  const int count;

  /**
   * @brief 起始位置
   */
  const Location* orig_loc;

  /**
   * @brief 目标位置
   */
  const Location* dest_loc;

  Vehicle(const Carrier* carrier, const int ind, const VehicleModel* vehicle_model, int count,
          const Location* orig_loc, const Location* dest_loc)
      : carrier(carrier),
        ind(ind),
        vehicle_model(vehicle_model),
        count(count),
        orig_loc(orig_loc),
        dest_loc(dest_loc) {}

  ~Vehicle() = default;

  bool operator==(const Vehicle& other) const { return this->ind == other.ind; }

  const DistMatrixCode* get_dist_matrix_code() const {
    return this->vehicle_model->dist_matrix_code;
  };

  const DistMatrix* get_dist_matrix() const { return this->vehicle_model->dist_matrix; }

  bool unusable() const { return this->count == 0; }
};

namespace std {
template <>
struct hash<Vehicle> {
  size_t operator()(const Vehicle& v) const {
    // 使用 ind 作为哈希值（因为 operator== 只比较 ind）
    return std::hash<int>{}(v.ind);
  }
};
}  // namespace std

struct CompareByVehicleInd {
  bool operator()(const Vehicle* vehicle_a, const Vehicle* vehicle_b) const noexcept {
    return vehicle_a->ind < vehicle_b->ind;
  }
};
