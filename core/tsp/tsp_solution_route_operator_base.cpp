#include <vector>
#include <tuple>

#include <spdlog/spdlog.h>

#include "time_window_utils.h"
#include "tsp_solution_route_operator.h"

TspSolRouteOperator::TspSolRouteOperator(TspContext *&tsp_context,
                                         std::unique_ptr<TspSolutionObjUtil> tsp_sol_obj_util,
                                         std::unique_ptr<TspConstraintHardUtils> tsp_constraint_hard_utils)
{
    this->tsp_context = tsp_context;
    this->tsp_sol_obj_util = std::move(tsp_sol_obj_util);
    this->tsp_constraint_hard_utils = std::move(tsp_constraint_hard_utils);
}

TspSolRouteOperator::~TspSolRouteOperator()
{
    this->tsp_context = nullptr;
}

void TspSolRouteOperator::insert_node(int route_ind, TspLocation *loc, std::unique_ptr<TspSolutionRoute> &route)
{
    std::unique_ptr<TspSolutionNode> &pre_node = route->nodes[route_ind - 1];
    long cost_dist = this->tsp_context->cost_matrix->get_dist(pre_node->matrix_ind, loc->matrix_ind);
    long cost_time = this->tsp_context->cost_matrix->get_time(pre_node->matrix_ind, loc->matrix_ind);
    std::unique_ptr<TspSolutionNode> cur_node = std::make_unique<TspSolutionNode>(loc);
    cur_node->cost_dist = cost_dist;
    cur_node->cost_time = cost_time;
    cur_node->time_windows = TimeWindowUtils::forward_infer(pre_node->time_windows, cost_time, loc->work_time, loc->time_windows);

    // update route attr
    route->attr->total_cost_dist += cost_dist;
    route->attr->total_cost_time += cost_time;
    route->attr->wait_times[route_ind] = cur_node->time_windows[0]->wait_time;
    route->attr->over_times[route_ind] = cur_node->time_windows[0]->over_time;
    // next node is last node
    if ((this->tsp_context->visit_loc_count - 2) == route_ind)
    {
        TspSolutionNode *next_node = route->last_node();
        long to_next_node_cost_dist = this->tsp_context->cost_matrix->get_dist(loc->matrix_ind, next_node->matrix_ind);
        long to_next_node_cost_time = this->tsp_context->cost_matrix->get_time(loc->matrix_ind, next_node->matrix_ind);

        next_node->cost_dist = to_next_node_cost_dist;
        next_node->cost_time = to_next_node_cost_time;
        TspLocation *last_loc = this->tsp_context->last_loc;
        next_node->time_windows = TimeWindowUtils::forward_infer(cur_node->time_windows, to_next_node_cost_time, last_loc->work_time, last_loc->time_windows);
        // update route attr
        route->attr->total_cost_dist += to_next_node_cost_dist;
        route->attr->total_cost_time += to_next_node_cost_time;
        route->attr->wait_times[tsp_context->visit_loc_count - 1] = next_node->time_windows[0]->wait_time;
        route->attr->over_times[tsp_context->visit_loc_count - 1] = next_node->time_windows[0]->over_time;
    }

    route->nodes[route_ind] = std::move(cur_node);

    // 重新计算目标函数值
    this->tsp_sol_obj_util->compute(this->tsp_context, route->attr);
}

void TspSolRouteOperator::temp_infer_time_windows(int pre_begin_route_ind, int next_end_route_ind, std::vector<std::tuple<int, int>> &wait_upate_route_indices, std::unique_ptr<TspSolutionRouteAttr> &route_attr, std::unique_ptr<TspSolutionRoute> &route)
{
    int view_route_ind = pre_begin_route_ind;

    std::vector<TspLocation *> &locations = this->tsp_context->locations;
    CostMatrix *&cost_matrix = this->tsp_context->cost_matrix;
    std::vector<std::unique_ptr<TspSolutionNode>> &nodes = route->nodes;
    std::vector<std::unique_ptr<TimeWindowPlan>> pre_node_time_windows = nodes[pre_begin_route_ind]->deep_copy_time_windows();
    int pre_matrix_ind = nodes[pre_begin_route_ind]->matrix_ind;

    for (const auto &route_ind_range : wait_upate_route_indices)
    {
        int begin_route_ind = std::get<0>(route_ind_range);
        int end_route_ind = std::get<1>(route_ind_range);
        if (begin_route_ind <= end_route_ind)
        {
            for (int u = begin_route_ind; u <= end_route_ind; u++)
            {
                ++view_route_ind;
                int cur_matrix_ind = nodes[u]->matrix_ind;
                int cur_loc_ind = nodes[u]->loc_ind;
                long cost_time = cost_matrix->get_time(pre_matrix_ind, cur_matrix_ind);
                pre_node_time_windows = TimeWindowUtils::forward_infer(pre_node_time_windows, cost_time, locations[cur_loc_ind]->work_time, locations[cur_loc_ind]->time_windows);
                route_attr->wait_times[view_route_ind] = pre_node_time_windows[0]->wait_time;
                route_attr->over_times[view_route_ind] = pre_node_time_windows[0]->over_time;
                pre_matrix_ind = cur_matrix_ind;
            }
        }
        else
        {
            for (int u = begin_route_ind; u >= end_route_ind; u--)
            {
                ++view_route_ind;
                int cur_matrix_ind = nodes[u]->matrix_ind;
                int cur_loc_ind = nodes[u]->loc_ind;
                long cost_time = cost_matrix->get_time(pre_matrix_ind, cur_matrix_ind);
                pre_node_time_windows = TimeWindowUtils::forward_infer(pre_node_time_windows, cost_time, locations[cur_loc_ind]->work_time, locations[cur_loc_ind]->time_windows);
                route_attr->wait_times[view_route_ind] = pre_node_time_windows[0]->wait_time;
                route_attr->over_times[view_route_ind] = pre_node_time_windows[0]->over_time;
                pre_matrix_ind = cur_matrix_ind;
            }
        }
    }
    for (int u = next_end_route_ind; u < route->loc_count; ++u)
    {
        ++view_route_ind;
        int cur_matrix_ind = nodes[u]->matrix_ind;
        int cur_loc_ind = nodes[u]->loc_ind;
        long cost_time = cost_matrix->get_time(pre_matrix_ind, cur_matrix_ind);
        pre_node_time_windows = TimeWindowUtils::forward_infer(pre_node_time_windows, cost_time, locations[cur_loc_ind]->work_time, locations[cur_loc_ind]->time_windows);
        route_attr->wait_times[view_route_ind] = pre_node_time_windows[0]->wait_time;
        route_attr->over_times[view_route_ind] = pre_node_time_windows[0]->over_time;
        pre_matrix_ind = cur_matrix_ind;
    }
}

void TspSolRouteOperator::infer_time_windows(int pre_begin_route_ind, std::unique_ptr<TspSolutionRoute> &route)
{

    std::vector<TspLocation *> &locations = this->tsp_context->locations;
    std::vector<std::unique_ptr<TspSolutionNode>> &nodes = route->nodes;

    for (int u = pre_begin_route_ind + 1; u < route->loc_count; ++u)
    {
        std::unique_ptr<TspSolutionNode> &cur_node = route->nodes[u];

        TspLocation *&cur_loc = locations[cur_node->loc_ind];
        cur_node->time_windows = TimeWindowUtils::forward_infer(nodes[u - 1]->time_windows, cur_node->cost_time, cur_loc->work_time, cur_loc->time_windows);

        route->attr->wait_times[u] = cur_node->time_windows[0]->wait_time;
        route->attr->over_times[u] = cur_node->time_windows[0]->over_time;
    }
}
