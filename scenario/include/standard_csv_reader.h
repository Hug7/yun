/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <string>
#include <vector>
#include <format>
#include <filesystem>

#include <spdlog/spdlog.h>

#include "c_rapidcsv.h"
#include "c_csv_utils.h"
#include "c_file_utils.h"
#include "c_time_utils.h"
#include "se_schema.h"
#include "se_scenario.h"

/**
 * @brief 标准模型读取
 */
class StandardCsvReader
{
private:
    /**
     * @brief 根目录
     */
    const std::string root_dir;
    /**
     * @brief 读取维度
     * @return DimensionManager* 维度管理器
     */
    DimensionManager *loading_dimension();
    /**
     * @brief 读取标签
     * @return LabelManager* 标签管理器
     */
    LabelManager *loading_label();
    /**
     * @brief 读取站点
     * @param label_manager 标签管理器
     * @return LocationManager* 站点管理器
     */
    LocationManager *loading_location(LabelManager *label_manager);
    /**
     * @brief 读取距离矩阵
     * @param location_manager 站点管理器
     * @return DistMatrixManager* 距离矩阵管理器
     */
    DistMatrixManager *loading_dist_matrix(LocationManager *location_manager);
    /**
     * @brief 读取车型
     * @param dimension_manager 维度管理器
     * @param label_manager 标签管理器
     * @param dist_matrix_manager 距离矩阵管理器
     * @return VehicleModelManager* 车型管理器
     */
    VehicleModelManager *loading_vehicle_model(DimensionManager *dimension_manager,
                                               LabelManager *label_manager,
                                               DistMatrixManager *dist_matrix_manager);
    /**
     * @brief 读取承运商
     * @param label_manager 标签管理器
     * @return CarrierManager* 承运商管理器
     */
    CarrierManager *loading_carrier(LabelManager *label_manager);
    /**
     * @brief 读取订单
     * @param location_manager 站点管理器
     * @param dimension_manager 维度管理器
     * @param label_manager 标签管理器
     */
    CargoOrderManager *loading_cargo_order(LocationManager *location_manager,
                                           DimensionManager *dimension_manager,
                                           LabelManager *label_manager);
    /**
     * @brief 读取车辆
     * @param carrier_manager 承运商管理器
     * @param vehicle_model_manager 车型管理器
     * @param location_manager 站点管理器
     */
    void loading_vehicle(CarrierManager *carrier_manager,
                         VehicleModelManager *vehicle_model_manager,
                         LocationManager *location_manager);
    /**
     * @brief 读取车辆可用车辆
     * @param carrier_manager 承运商管理器
     * @param location_manager 站点管理器
     */
    void loading_available_vehicle(CarrierManager *carrier_manager,
                                   LocationManager *location_manager);
    /**
     * @brief 读取工作计划
     * @param location_manager 站点管理器
     * @param dimension_manager 维度管理器
     */
    void loading_work_plan(LocationManager *location_manager, DimensionManager *dimension_manager);
    /**
     * @brief 读取参数
     */
    Parameter *loading_parameter();

public:
    StandardCsvReader(const std::string &root_dir) : root_dir(root_dir) {}

    /**
     * @brief 加载场景
     * @return Scenario* 场景
     */
    Scenario *loading_scenario();
};