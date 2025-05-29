#ifndef TSP_SOLUTION_CHECK_H
#define TSP_SOLUTION_CHECK_H

#include <cassert>

#include "tsp_solution_route.h"
#include "tsp_solution_route_operator.h"

void do_check(TspSolRouteOperator *&tsp_operator, std::unique_ptr<TspSolutionRoute> &ori_route)
{
    std::unique_ptr<TspSolutionRoute> route = ori_route->deep_copy();
    CostMatrix *&cost_matrix = tsp_operator->tsp_context->cost_matrix;

    long total_cost_dist = 0;
    long total_cost_time = 0;
    for (int u = 1; u < route->loc_count; ++u) {
        route->nodes[u]->cost_dist = cost_matrix->get_dist(route->nodes[u - 1]->matrix_ind, route->nodes[u]->matrix_ind);
        route->nodes[u]->cost_time = cost_matrix->get_time(route->nodes[u - 1]->matrix_ind, route->nodes[u]->matrix_ind);
        total_cost_dist += route->nodes[u]->cost_dist;
        total_cost_time += route->nodes[u]->cost_time;
    }
    route->attr->total_cost_dist = total_cost_dist;
    route->attr->total_cost_time = total_cost_time;

    tsp_operator->tsp_sol_obj_util->compute(tsp_operator->tsp_context, route->attr);

    // spdlog::warn("route->attr->obj_val = {}", route->attr->obj_val);
    // spdlog::warn("ori_route->attr->obj_val = {}", ori_route->attr->obj_val);
    // spdlog::warn(std::abs(route->attr->obj_val - ori_route->attr->obj_val));
    assert(std::abs(route->attr->obj_val - ori_route->attr->obj_val) < 0.1);
    assert(std::abs(route->attr->total_cost_dist - ori_route->attr->total_cost_dist) < 0.1);
    assert(std::abs(route->attr->total_cost_time - ori_route->attr->total_cost_time) < 0.1);
    assert(std::abs(route->attr->penalty_val - ori_route->attr->penalty_val) < 0.1);
    for (int u = 0; u < route->loc_count; ++u)
    {
        assert(std::abs(route->nodes[u]->cost_dist - ori_route->nodes[u]->cost_dist) < 0.1);
        assert(std::abs(route->nodes[u]->cost_time - ori_route->nodes[u]->cost_time) < 0.1);
        assert(std::abs(static_cast<int>(route->nodes[u]->time_windows.size()) - static_cast<int>(ori_route->nodes[u]->time_windows.size())) == 0);
        assert(std::abs(route->attr->wait_times[u] - ori_route->attr->wait_times[u]) == 0);
        assert(std::abs(route->attr->over_times[u] - ori_route->attr->over_times[u]) == 0);
    }    
}

#endif // TSP_SOLUTION_CHECK_H
