#include <vector>
#include <tuple>

#include <spdlog/spdlog.h>

#include "tsp_solution_route_operator.h"


void TspSolRouteOperator::temp_swap(int route_ind_a, int route_ind_b, std::unique_ptr<TspSolutionRouteAttr> &route_attr, std::unique_ptr<TspSolutionRoute> &ori_route)
{
    /**
     * a-1,a,a+1,b-1,b,b+1 -> a-1,b,a+1,b-1,a,b+1
     * a-1,a,a+1(b-1),b,b+1 -> a-1,b,a+1(b-1),a,b+1
     * a-1,a,b,b+1 -> a-1,b,a,b+1
     */
    if (route_ind_a > route_ind_b)
    {
        int tmp = route_ind_a;
        route_ind_a = route_ind_b;
        route_ind_b = tmp;
    }

    const std::vector<std::unique_ptr<TspSolutionNode>> &nodes = ori_route->nodes;
    int matrix_ind_a = nodes[route_ind_a]->matrix_ind;
    int matrix_ind_a_l = nodes[route_ind_a - 1]->matrix_ind;
    int matrix_ind_a_r = nodes[route_ind_a + 1]->matrix_ind;
    int matrix_ind_b = nodes[route_ind_b]->matrix_ind;
    int matrix_ind_b_l = nodes[route_ind_b - 1]->matrix_ind;
    int matrix_ind_b_r = nodes[route_ind_b + 1]->matrix_ind;

    // 拆了选中节点的信息
    int total_cost_dist = ori_route->attr->total_cost_dist;
    int total_cost_time = ori_route->attr->total_cost_time;

    total_cost_dist -= nodes[route_ind_a]->cost_dist;
    total_cost_dist -= nodes[route_ind_b]->cost_dist;
    total_cost_dist -= nodes[route_ind_b + 1]->cost_dist;
    total_cost_time -= nodes[route_ind_a]->cost_time;
    total_cost_time -= nodes[route_ind_b]->cost_time;
    total_cost_time -= nodes[route_ind_b + 1]->cost_time;

    // 将选中的节点插入到目标位置
    total_cost_dist += this->tsp_context->cost_matrix->get_dist(matrix_ind_a_l, matrix_ind_b);
    total_cost_dist += this->tsp_context->cost_matrix->get_dist(matrix_ind_a, matrix_ind_b_r);

    total_cost_time += this->tsp_context->cost_matrix->get_time(matrix_ind_a_l, matrix_ind_b);
    total_cost_time += this->tsp_context->cost_matrix->get_time(matrix_ind_a, matrix_ind_b_r);

    if (route_ind_b - route_ind_a > 1)
    {
        total_cost_dist -= nodes[route_ind_a + 1]->cost_dist;
        total_cost_time -= nodes[route_ind_a + 1]->cost_time;

        total_cost_dist += this->tsp_context->cost_matrix->get_dist(matrix_ind_b, matrix_ind_a_r);
        total_cost_dist += this->tsp_context->cost_matrix->get_dist(matrix_ind_b_l, matrix_ind_a);

        total_cost_time += this->tsp_context->cost_matrix->get_time(matrix_ind_b, matrix_ind_a_r);
        total_cost_time += this->tsp_context->cost_matrix->get_time(matrix_ind_b_l, matrix_ind_a);
    }
    else
    {
        total_cost_dist += this->tsp_context->cost_matrix->get_dist(matrix_ind_b, matrix_ind_a);

        total_cost_time += this->tsp_context->cost_matrix->get_time(matrix_ind_b, matrix_ind_a);
    }

    route_attr->total_cost_dist = total_cost_dist;
    route_attr->total_cost_time = total_cost_time;

    // 更新时间窗
    std::vector<std::tuple<int, int>> wait_upate_route_indices;
    wait_upate_route_indices.push_back(std::make_tuple(route_ind_b, route_ind_b));
    if (route_ind_b - route_ind_a > 1)
    {
        wait_upate_route_indices.push_back(std::make_tuple(route_ind_a + 1, route_ind_b - 1));
    }
    wait_upate_route_indices.push_back(std::make_tuple(route_ind_a, route_ind_a));
    wait_upate_route_indices.push_back(std::make_tuple(route_ind_b + 1, ori_route->loc_count - 1));

    temp_infer_time_windows(route_ind_a - 1, ori_route->loc_count, wait_upate_route_indices, route_attr, ori_route);
}

void TspSolRouteOperator::swap(int route_ind_a, int route_ind_b, std::unique_ptr<TspSolutionRoute> &ori_route)
{
    /**
     * a-1,a,a+1,b-1,b,b+1 -> a-1,b,a+1,b-1,a,b+1
     * a-1,a,a+1(b-1),b,b+1 -> a-1,b,a+1(b-1),a,b+1
     * a-1,a,b,b+1 -> a-1,b,a,b+1
     */
    if (route_ind_a > route_ind_b)
    {
        int tmp = route_ind_a;
        route_ind_a = route_ind_b;
        route_ind_b = tmp;
    }

    std::vector<std::unique_ptr<TspSolutionNode>> &nodes = ori_route->nodes;
    int matrix_ind_a = nodes[route_ind_a]->matrix_ind;
    int matrix_ind_a_l = nodes[route_ind_a - 1]->matrix_ind;
    int matrix_ind_a_r = nodes[route_ind_a + 1]->matrix_ind;
    int matrix_ind_b = nodes[route_ind_b]->matrix_ind;
    int matrix_ind_b_l = nodes[route_ind_b - 1]->matrix_ind;
    int matrix_ind_b_r = nodes[route_ind_b + 1]->matrix_ind;

    // 拆了选中节点的信息
    int total_cost_dist = ori_route->attr->total_cost_dist;
    int total_cost_time = ori_route->attr->total_cost_time;

    total_cost_dist -= nodes[route_ind_a]->cost_dist;
    total_cost_dist -= nodes[route_ind_b]->cost_dist;
    total_cost_dist -= nodes[route_ind_b + 1]->cost_dist;
    total_cost_time -= nodes[route_ind_a]->cost_time;
    total_cost_time -= nodes[route_ind_b]->cost_time;
    total_cost_time -= nodes[route_ind_b + 1]->cost_time;

    // 将选中的节点插入到目标位置
    nodes[route_ind_b]->cost_dist = this->tsp_context->cost_matrix->get_dist(matrix_ind_a_l, matrix_ind_b);
    nodes[route_ind_b + 1]->cost_dist = this->tsp_context->cost_matrix->get_dist(matrix_ind_a, matrix_ind_b_r);

    total_cost_dist += nodes[route_ind_b]->cost_dist + nodes[route_ind_b + 1]->cost_dist;

    nodes[route_ind_b]->cost_time = this->tsp_context->cost_matrix->get_time(matrix_ind_a_l, matrix_ind_b);
    nodes[route_ind_b + 1]->cost_time = this->tsp_context->cost_matrix->get_time(matrix_ind_a, matrix_ind_b_r);

    total_cost_time += nodes[route_ind_b]->cost_time + nodes[route_ind_b + 1]->cost_time;

    if (route_ind_b - route_ind_a > 1)
    {
        total_cost_dist -= nodes[route_ind_a + 1]->cost_dist;
        total_cost_time -= nodes[route_ind_a + 1]->cost_time;

        nodes[route_ind_a + 1]->cost_dist = this->tsp_context->cost_matrix->get_dist(matrix_ind_b, matrix_ind_a_r);
        nodes[route_ind_a]->cost_dist = this->tsp_context->cost_matrix->get_dist(matrix_ind_b_l, matrix_ind_a);

        total_cost_dist += nodes[route_ind_a + 1]->cost_dist + nodes[route_ind_a]->cost_dist;

        nodes[route_ind_a + 1]->cost_time = this->tsp_context->cost_matrix->get_time(matrix_ind_b, matrix_ind_a_r);
        nodes[route_ind_a]->cost_time = this->tsp_context->cost_matrix->get_time(matrix_ind_b_l, matrix_ind_a);

        total_cost_time += nodes[route_ind_a + 1]->cost_time + nodes[route_ind_a]->cost_time;
    } else
    {
        nodes[route_ind_a]->cost_dist = this->tsp_context->cost_matrix->get_dist(matrix_ind_b, matrix_ind_a);

        total_cost_dist += nodes[route_ind_a]->cost_dist;

        nodes[route_ind_a]->cost_time = this->tsp_context->cost_matrix->get_time(matrix_ind_b, matrix_ind_a);

        total_cost_time += nodes[route_ind_a]->cost_time;
    }

    std::unique_ptr<TspSolutionNode> tmp_node = std::move(nodes[route_ind_a]);
    nodes[route_ind_a] = std::move(nodes[route_ind_b]);
    nodes[route_ind_b] = std::move(tmp_node);

    ori_route->attr->total_cost_dist = total_cost_dist;
    ori_route->attr->total_cost_time = total_cost_time;

    // 更新时间窗
    infer_time_windows(route_ind_a - 1, ori_route);
}
