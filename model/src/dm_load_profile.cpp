/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#include "dm_load_profile.h"

// ====== implement of LoadAttrBase ======
LoadRouteProfile::LoadRouteProfile(const Scenario *scenario)
{
    this->dirty_marks = std::make_unique<Bitset>(static_cast<size_t>(LoadRouteProfileField::FIELDS_COUNT));
    this->peak_load_dims = scenario->dimension_manager->empty_dim_values();
    this->pick_loc_labelset_value_bitset = scenario->label_manager->location_labelset->empty_labelset_value_bitset();
    this->drop_loc_labelset_value_bitset = scenario->label_manager->location_labelset->empty_labelset_value_bitset();
    this->order_labelset_value_bitset = scenario->label_manager->order_labelset->empty_labelset_value_bitset();
    this->available_vehicle_bitset = scenario->carrier_manager->empty_vehicle_bitset();
}

bool LoadRouteProfile::get_set_dirty_mark(LoadRouteProfileField field)
{
    const int field_ind = static_cast<int>(field);
    if (this->dirty_marks->test(field_ind))
    {
        return true;
    }
    this->dirty_marks->set(field_ind);
    return false;
}

bool LoadRouteProfile::get_dirty_mark(const int field_ind)
{
    return this->dirty_marks->test(field_ind);
}

void LoadRouteProfile::set_dirty_mark(const int field_ind)
{
    this->dirty_marks->set(field_ind);
}

// ====== implement of LoadConstraintProfile ======
void LoadConstraintProfile::add_hard_constr_score(std::unique_ptr<HardConstrScore> score)
{
    if (score == nullptr)
    {
        return;
    }
    this->total_hard_penalty += score->get_score();
    this->hard_constr_scores.push_back(std::move(score));
}

void LoadConstraintProfile::add_soft_constr_score(std::unique_ptr<SoftConstrScore> score)
{
    this->total_soft_penalty += score->get_score();
    this->soft_constr_scores.push_back(std::move(score));
}

void LoadConstraintProfile::add_cost_constr_score(std::unique_ptr<CostConstrScore> score)
{
    this->total_cost += score->get_score();
    this->cost_constr_scores.push_back(std::move(score));
}
