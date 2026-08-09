/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <vector>
#include <memory>

#include "bd_common.h"
#include "bd_label.h"
#include "bd_dist_matrix.h"
#include "se_scenario.h"
#include "dm_score.h"

enum class LoadRouteProfileField
{
    PEAK_LOAD_DIMS,
    PICK_LOC_LABELSET_VALUE_BITSET,
    DROP_LOC_LABELSET_VALUE_BITSET,
    ORDER_LABELSET_VALUE_BITSET,
    AVAILABLE_VEHICLE_BITSET,
    PICK_NODE_COUNT,
    DROP_NODE_COUNT,
    FIELDS_COUNT  // sentinel does not participate in dirty mark
};

/**
 * @brief: profile of a load
 */
class LoadRouteProfile
{
public:
    /**
     * @brief: dirty mark of the profile
     */
    std::unique_ptr<Bitset> dirty_marks;
    /**
     * @brief: peak loading value of all dimensions for the load
     */
    std::vector<long> peak_load_dims;
    /**
     * @brief: bitset of labelset value for the pick location
     */
    std::unique_ptr<LabelsetValueBitset> pick_loc_labelset_value_bitset;
    /**
     * @brief: bitset of labelset value for the drop location
     */
    std::unique_ptr<LabelsetValueBitset> drop_loc_labelset_value_bitset;
    /**
     * @brief: bitset of labelset value for the orders
     */
    std::unique_ptr<LabelsetValueBitset> order_labelset_value_bitset;
    /**
     * @brief: bitset of available vehicles
     */
    std::unique_ptr<Bitset> available_vehicle_bitset;

    int pick_node_count;

    int drop_node_count;

    long total_dist;

    LoadRouteProfile(const Scenario *scenario);

    bool get_set_dirty_mark(LoadRouteProfileField field);

    bool get_dirty_mark(const int field_ind);

    void set_dirty_mark(const int field_ind);


};

/**
 * @brief: constraint profile of a load
 */
class LoadConstraintProfile
{
public:
    /**
     * @brief: total penalty of hard constraint for load
     */
    double total_hard_penalty;
    /**
     * @brief: total penalty of soft constraint for load
     */
    double total_soft_penalty;
    /**
     * @brief: total penalty of cost constraint for load
     */
    double total_cost;

    std::vector<std::unique_ptr<HardConstrScore>> hard_constr_scores;

    std::vector<std::unique_ptr<SoftConstrScore>> soft_constr_scores;

    std::vector<std::unique_ptr<CostConstrScore>> cost_constr_scores;

    LoadConstraintProfile() {}

    void add_hard_constr_score(std::unique_ptr<HardConstrScore> score);

    void add_soft_constr_score(std::unique_ptr<SoftConstrScore> score);

    void add_cost_constr_score(std::unique_ptr<CostConstrScore> score);
};
