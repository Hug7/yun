/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#include "dm_load.h"

// ====== implement of Load ======
Load::Load(const Scenario *scenario) : scenario(scenario)
{
    Location *default_loc = scenario->location_manager->get_default_location();
    this->first_node = std::make_unique<Node>(ActivityType::START, default_loc);
    auto end_node = std::make_unique<Node>(ActivityType::END, default_loc);
    end_node->prev = this->first_node.get();
    this->first_node->next = std::move(end_node);
    this->last_node = end_node.get();
    this->route_profile = std::make_unique<LoadRouteProfile>(scenario);
    this->constr_profile = std::make_unique<LoadConstraintProfile>();
}

void Load::change_vehicle(Vehicle *vehicle)
{
    this->vehicle = vehicle;
    // todo update route profile when change vehicle
    // todo 更换车辆后始发站点和结束站点变更后更新dirty mark
}

long Load::get_total_dist()
{
    long total_dist = 0;
    Node *tail_node = this->last_node;
    while (tail_node->hase_prev())
    {
        total_dist += tail_node->travel_dist;
        tail_node = tail_node->prev;
    }
    return total_dist;
}

void Load::update_node_dist_time()
{
    const DistMatrix *dist_matrix = this->vehicle->get_dist_matrix();
    Node *tail_node = this->last_node;
    while (tail_node->hase_prev())
    {
        Node *prev_node = tail_node->prev;
        auto dist_time = dist_matrix->get_dist_time(prev_node->loc->ind, tail_node->loc->ind);

        tail_node->travel_dist = dist_time.dist;
        tail_node->travel_time = dist_time.time;
        tail_node = prev_node;
    }
}
