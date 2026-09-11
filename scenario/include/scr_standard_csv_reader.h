/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <spdlog/spdlog.h>

#include <memory>
#include <string>
#include <utility>

#include "se_scenario.h"

/**
 * @brief 标准模型读取
 */
class StandardCsvReader {
 private:
  /**
   * @brief 根目录
   */
  const std::string root_dir;
  /**
   * @brief 日志器，由 SolverContext 传入，本次请求专属
   */
  const std::shared_ptr<spdlog::logger> logger;
  /**
   * @brief 读取维度
   * @return DimensionManager* 维度管理器
   */
  [[nodiscard]] DimensionManager* loading_dimension() const;
  /**
   * @brief 读取标签
   * @return LabelManager* 标签管理器
   */
  [[nodiscard]] LabelManager* loading_label() const;
  /**
   * @brief 读取站点
   * @param label_manager 标签管理器
   * @param dim_manager 维度管理器
   * @return LocationManager* 站点管理器
   */
  [[nodiscard]] LocationManager* loading_location(LabelManager* label_manager,
                                                  DimensionManager* dim_manager) const;
  /**
   * @brief 读取距离矩阵
   * @param location_manager 站点管理器
   * @return DistMatrixManager* 距离矩阵管理器
   */
  [[nodiscard]] DistMatrixManager* loading_dist_matrix(LocationManager* location_manager) const;
  /**
   * @brief 读取车型
   * @param dimension_manager 维度管理器
   * @param label_manager 标签管理器
   * @param dist_matrix_manager 距离矩阵管理器
   * @return VehicleModelManager* 车型管理器
   */
  [[nodiscard]] VehicleModelManager* loading_vehicle_model(
      DimensionManager* dimension_manager, LabelManager* label_manager,
      DistMatrixManager* dist_matrix_manager) const;
  /**
   * @brief 读取承运商
   * @param label_manager 标签管理器
   * @return CarrierManager* 承运商管理器
   */
  [[nodiscard]] CarrierManager* loading_carrier(LabelManager* label_manager) const;
  /**
   * @brief 读取订单
   * @param location_manager 站点管理器
   * @param dimension_manager 维度管理器
   * @param label_manager 标签管理器
   */
  [[nodiscard]] CargoOrderManager* loading_cargo_order(LocationManager* location_manager,
                                                       DimensionManager* dimension_manager,
                                                       LabelManager* label_manager) const;
  /**
   * @brief 读取车辆
   * @param carrier_manager 承运商管理器
   * @param vehicle_model_manager 车型管理器
   * @param location_manager 站点管理器
   */
  void loading_vehicle(CarrierManager* carrier_manager, VehicleModelManager* vehicle_model_manager,
                       LocationManager* location_manager) const;
  /**
   * @brief 读取车辆可用车辆
   * @param carrier_manager 承运商管理器
   * @param location_manager 站点管理器
   */
  void loading_available_vehicle(CarrierManager* carrier_manager,
                                 LocationManager* location_manager) const;
  /**
   * @brief 读取工作计划
   * @param location_manager 站点管理器
   * @param dimension_manager 维度管理器
   */
  void loading_work_plan(LocationManager* location_manager,
                         DimensionManager* dimension_manager) const;

 public:
  explicit StandardCsvReader(std::string root_dir, std::shared_ptr<spdlog::logger> logger)
      : root_dir(std::move(root_dir)), logger(std::move(logger)) {}

  /**
   * @brief 加载场景
   * @return Scenario* 场景
   */
  Scenario* loading_scenario();
};
