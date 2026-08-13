/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <string>

#include "bd_vehicle_model.h"
#include "bd_location.h"

class Carrier;

/**
 * @brief vehicle models with carrier
 */
class Vehicle
{
public:
    /**
     * @brief 承运商
     */
    const Carrier *carrier;

    /**
     * @brief 车辆索引
     */
    const int ind;

    /**
     * @brief 车型
     */
    const VehicleModel *vehicle_model;

    /**
     * @brief 车辆数量
     */
    int count;

    /**
     * @brief 起始位置
     */
    const Location *orig_loc;

    /**
     * @brief 目标位置
     */
    const Location *dest_loc;

    Vehicle(const Carrier *carrier,
            const int ind,
            const VehicleModel *vehicle_model,
            int count,
            const Location *orig_loc,
            const Location *dest_loc) : carrier(carrier),
                                        ind(ind),
                                        vehicle_model(vehicle_model),
                                        count(count),
                                        orig_loc(orig_loc),
                                        dest_loc(dest_loc) {}

    ~Vehicle() = default;

    inline const DistMatrixCode *get_dist_matrix_code() {return this->vehicle_model->dist_matrix_code;};
    inline const DistMatrix *get_dist_matrix() {return this->vehicle_model->dist_matrix;}
};

struct CompareByVehicleInd
{

    bool operator()(const Vehicle *vehicle_a, const Vehicle *vehicle_b) const noexcept
    {
        return vehicle_a->ind < vehicle_b->ind;
    }
};

class VehicleResource
{
public:
    /**
     * @brief 车辆可用
     */
    std::vector<int> usable;

    /**
     * @brief 已使用资源
     */
    std::vector<int> usage;

    VehicleResource(std::vector<int> &usable, std::vector<int> &usage) : usable(usable), usage(usage) {}
};
