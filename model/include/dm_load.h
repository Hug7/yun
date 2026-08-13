/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <memory>

#include "bd_vehicle.h"
#include "se_scenario.h"
#include "dm_load_profile.h"
#include "dm_activity.h"
#include "dm_node.h"

/**
 * @brief load
 * @details a complete delivery plan for orders
 */
class Load
{
public:
    /**
     * @brief scenario
     */
    const Scenario *scenario;
    /**
     * @brief pervious dist matrix code
     */
    const DistMatrixCode *prev_dist_matrix_code;
    /**
     * @brief vehicle
     */
    Vehicle *vehicle;
    /**
     * @brief fisrt node of the load
     */
    Node::UPtr first_node;
    /**
     * @brief last node of the load
     */
    Node *last_node;
    /**
     * @brief route profile of the load
     */
    LoadRouteProfile::UPtr route_profile;
    /**
     * @brief constraint profile of the load
     */
    LoadConstrProfile::UPtr constr_profile;

    Load(const Scenario *scenario);

    void change_vehicle(Vehicle *vehicle);

    long get_total_dist();

    virtual void update_node_dist_time();

    virtual void update_start_node_dist_time();

    virtual void update_end_node_dist_time();

    virtual const std::vector<long> &get_peak_load_dims() = 0;

    virtual int get_pick_node_count() = 0;

    virtual int get_drop_node_count() = 0;
};

/**
 * @brief the load of SPMD VRP pattern
 */
class LoadSPMD : public Load
{
public:
    LoadSPMD(const Scenario *scenario) : Load(scenario) {};

    int get_pick_node_count() override;

    int get_drop_node_count() override;

    const std::vector<long> &get_peak_load_dims() override;
};
