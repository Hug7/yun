/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#include <filesystem>

#include "c_file_utils.h"
#include "se_scenario.h"
#include "standard_csv_reader.h"

Scenario* StandardCsvReader::loading_scenario() {
  spdlog::info("Loading scenario from files path: {}", this->root_dir);
  if (!FileUtils::file_exists(this->root_dir)) {
    spdlog::error("The files path {} is not exists!", this->root_dir);
    throw std::filesystem::filesystem_error(
        this->root_dir, std::error_code(static_cast<int>(std::errc::no_such_file_or_directory),
                                        std::generic_category()));
  }
  // 读取维度管理器
  DimensionManager* dimension_manager = this->loading_dimension();
  // 读取标签管理器
  LabelManager* label_manager = this->loading_label();
  // 读取站点管理器
  LocationManager* location_manager = this->loading_location(label_manager);
  // 读取work plan
  this->loading_work_plan(location_manager, dimension_manager);
  // 读取距离矩阵
  DistMatrixManager* dist_matrix_manager = this->loading_dist_matrix(location_manager);
  // 读取车型管理器
  VehicleModelManager* vehicle_model_manager =
      this->loading_vehicle_model(dimension_manager, label_manager, dist_matrix_manager);
  // 读取承运商
  CarrierManager* carrier_manager = this->loading_carrier(label_manager);
  // 读取车辆
  this->loading_vehicle(carrier_manager, vehicle_model_manager, location_manager);
  // 读取可用车辆
  this->loading_available_vehicle(carrier_manager, location_manager);
  // 读取订单
  CargoOrderManager* cargo_order_manager =
      this->loading_cargo_order(location_manager, dimension_manager, label_manager);
  // 读取参数
  Parameter* parameter = this->loading_parameter();

  spdlog::info("Loading scenario finished!");

  // 构造模型
  Scenario* scenario = new Scenario();
  scenario->set_parameter(parameter);
  scenario->set_dimension_manager(dimension_manager);
  scenario->set_label_manager(label_manager);
  scenario->set_location_manager(location_manager);
  scenario->set_vehicle_model_manager(vehicle_model_manager);
  scenario->set_dist_matrix_manager(dist_matrix_manager);
  scenario->set_carrier_manager(carrier_manager);
  scenario->set_cargo_order_manager(cargo_order_manager);

  return scenario;
}
