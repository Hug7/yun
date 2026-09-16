/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <memory>
#include <unordered_map>
#include <vector>

#include "bdm_carrier.h"
#include "bdm_vehicle.h"
#include "pdm_order.h"

class VehicleUsage {
 public:
  using UPtr = std::unique_ptr<VehicleUsage>;
  using VecUPtr = std::vector<UPtr>;
  /**
   * @brief 车辆
   */
  const Vehicle* vehicle;
  /**
   * @brief 最大可用数量
   */
  const int max_usable;
  /**
   * @brief 已使用数量
   */
  int cur_usage;

  explicit VehicleUsage(const Vehicle* vehicle, const int max_usable, const int cur_usage)
      : vehicle(vehicle), max_usable(max_usable), cur_usage(cur_usage) {};

  explicit VehicleUsage(const Vehicle* vehicle)
      : vehicle(vehicle), max_usable(vehicle->count), cur_usage(0) {};

  [[nodiscard]] UPtr deep_copy() const;

  void occupy_vehicle();

  void release_vehicle();
};

/**
 * @brief 车辆资源
 */
class VehicleResource {
 public:
  using UPtr = std::unique_ptr<VehicleResource>;
  /**
   * @brief 车辆资源列表
   */
  const VehicleUsage::VecUPtr vehicle_usages;
  /**
   * @brief 车辆种类数
   */
  const int len;

  explicit VehicleResource(VehicleUsage::VecUPtr vehicle_usages)
      : vehicle_usages(std::move(vehicle_usages)), len(static_cast<int>(this->vehicle_usages.size())) {};

  [[nodiscard]] UPtr deep_copy() const;

  void occupy(const Vehicle* vehicle) const;

  void release(const Vehicle* vehicle) const;
};

/**
 * @brief 订单资源
 */
class OrderResource {
 public:
  using UPtr = std::unique_ptr<OrderResource>;
  /**
   * @brief 订单集合
   */
  const std::unordered_map<int, Order*> order_map;
  /**
   * @brief 订单数量
   */
  const int len;

  explicit OrderResource(std::unordered_map<int, Order*> order_map)
      : order_map(std::move(order_map)), len(static_cast<int>(order_map.size())) {};
};

/**
 * @brief 资源工厂
 */
namespace ResourceFactory {
/**
 * @brief 基于承运商创建车辆资源
 * @param carrier_manager 承运商管理器
 * @return 车辆资源
 */
VehicleResource::UPtr create_vehicle_resource(const CarrierManager* carrier_manager);
}  // namespace ResourceFactory
