/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "bd_cargo_order.h"
#include "bd_carrier.h"
#include "bd_dimension.h"
#include "bd_dist_matrix.h"
#include "bd_label.h"
#include "bd_location.h"
#include "bd_parameter.h"
#include "bd_vehicle_model.h"

/**
 * @brief 场景
 * @details 基础数据集合。设计原则约定：数据在加载+预处理后不应被修改
 */
class Scenario {
 public:
  Parameter* parameter;
  /**
   * @brief 维度管理器
   */
  DimensionManager* dimension_manager;
  /**
   * @brief 标签管理器
   */
  LabelManager* label_manager;
  /**
   * @brief 车型管理器
   */
  VehicleModelManager* vehicle_model_manager;
  /**
   * @brief 承运商管理器
   */
  CarrierManager* carrier_manager;
  /**
   * @brief 位置管理器
   */
  LocationManager* location_manager;
  /**
   * @brief 距离矩阵管理器
   */
  DistMatrixManager* dist_matrix_manager;
  /**
   * @brief 货物订单管理器
   */
  CargoOrderManager* cargo_order_manager;

  Scenario() {};

  ~Scenario();

  void set_parameter(Parameter* parameter);

  void set_dimension_manager(DimensionManager* dimension_manager);

  void set_label_manager(LabelManager* label_manager);

  void set_vehicle_model_manager(VehicleModelManager* vehicle_model_manager);

  void set_carrier_manager(CarrierManager* carrier_manager);

  void set_location_manager(LocationManager* location_manager);

  void set_dist_matrix_manager(DistMatrixManager* dist_matrix_manager);

  void set_cargo_order_manager(CargoOrderManager* cargo_order_manager);
};