#include <vector>
#include <tuple>

#include <spdlog/spdlog.h>

#include "tsp_solution_route_operator.h"

void TspSolRouteOperator::temp_k_opt(int begin_route_ind, int end_route_ind, std::vector<std::tuple<int, int>> &part_route_indices, std::unique_ptr<TspSolutionRouteAttr> &route_attr, std::unique_ptr<TspSolutionRoute> &route)
{
    long total_cost_dist = route->attr->total_cost_dist;
    long total_cost_time = route->attr->total_cost_time;

    CostMatrix *&cost_matrix = tsp_context->cost_matrix;
    std::vector<std::unique_ptr<TspSolutionNode>> &nodes = route->nodes;

    // 计算拆除 和 插入节点的信息
    int route_ind_a, route_ind_b;
    int pre_node_matrix_ind = nodes[begin_route_ind - 1]->matrix_ind;
    for (auto part_route_ind : part_route_indices)
    {
        std::tie(route_ind_a, route_ind_b) = part_route_ind;

        std::unique_ptr<TspSolutionNode> &node_a = nodes[route_ind_a];

        total_cost_dist -= node_a->cost_dist;
        total_cost_time -= node_a->cost_time;

        total_cost_dist += cost_matrix->get_dist(pre_node_matrix_ind, node_a->matrix_ind);
        total_cost_time += cost_matrix->get_time(pre_node_matrix_ind, node_a->matrix_ind);
        pre_node_matrix_ind = node_a->matrix_ind;

        if (route_ind_b >= route_ind_a)
        {
            pre_node_matrix_ind = nodes[route_ind_b]->matrix_ind;
            continue;
        }

        for (int u = route_ind_a - 1; u >= route_ind_b; --u)
        {
            std::unique_ptr<TspSolutionNode> &cur_node = nodes[u];

            total_cost_dist -= cur_node->cost_dist;
            total_cost_time -= cur_node->cost_time;

            total_cost_dist += cost_matrix->get_dist(pre_node_matrix_ind, cur_node->matrix_ind);
            total_cost_time += cost_matrix->get_time(pre_node_matrix_ind, cur_node->matrix_ind);
            pre_node_matrix_ind = cur_node->matrix_ind;
        }
    }
    total_cost_dist -= nodes[end_route_ind + 1]->cost_dist;
    total_cost_time -= nodes[end_route_ind + 1]->cost_time;

    total_cost_dist += cost_matrix->get_dist(pre_node_matrix_ind, nodes[end_route_ind + 1]->matrix_ind);
    total_cost_time += cost_matrix->get_time(pre_node_matrix_ind, nodes[end_route_ind + 1]->matrix_ind);

    route_attr->total_cost_dist = total_cost_dist;
    route_attr->total_cost_time = total_cost_time;

    // 推导时间窗
    temp_infer_time_windows(begin_route_ind - 1, end_route_ind + 1, part_route_indices, route_attr, route);
}

void TspSolRouteOperator::k_opt(std::vector<std::vector<int>> &part_route_indices, std::unique_ptr<TspSolutionRoute> &route)
{
    int begin_route_ind = part_route_indices[0][0];
    int end_route_ind = part_route_indices[0][1];

    std::vector<std::unique_ptr<TspSolutionNode>> &nodes = route->nodes;

    int wait_visited_count = end_route_ind - begin_route_ind + 2;

    CostMatrix *&cost_matrix = tsp_context->cost_matrix;

    // 先调整站点调整后的位置
    std::vector<std::unique_ptr<TspSolutionNode>> tmp_nodes(wait_visited_count);
    int route_ind_a, route_ind_b;
    int tmp_ind = -1;
    for (int k = 1; k < part_route_indices.size(); ++k)
    {
        route_ind_a = part_route_indices[k][0];
        route_ind_b = part_route_indices[k][1];
        if (route_ind_a <= route_ind_b)
        {
            for (int u = route_ind_a; u <= route_ind_b; ++u)
            {
                tmp_nodes[++tmp_ind] = std::move(nodes[u]);
            }
        }
        else
        {
            for (int u = route_ind_a; u >= route_ind_b; --u)
            {
                tmp_nodes[++tmp_ind] = std::move(nodes[u]);
            }
        }
    }
    tmp_nodes[++tmp_ind] = std::move(nodes[end_route_ind + 1]); // 把调整route片段的尾点加入便于计算

    // 更新距离和时间
    long total_cost_dist = route->attr->total_cost_dist;
    long total_cost_time = route->attr->total_cost_time;

    int pre_node_matrix_ind = nodes[begin_route_ind - 1]->matrix_ind;
    for (int u = 0; u < wait_visited_count; ++u)
    {
        total_cost_dist -= tmp_nodes[u]->cost_dist;
        total_cost_time -= tmp_nodes[u]->cost_time;

        tmp_nodes[u]->cost_dist = cost_matrix->get_dist(pre_node_matrix_ind, tmp_nodes[u]->matrix_ind);
        tmp_nodes[u]->cost_time = cost_matrix->get_time(pre_node_matrix_ind, tmp_nodes[u]->matrix_ind);

        total_cost_dist += tmp_nodes[u]->cost_dist;
        total_cost_time += tmp_nodes[u]->cost_time;

        pre_node_matrix_ind = tmp_nodes[u]->matrix_ind;

        nodes[begin_route_ind + u] = std::move(tmp_nodes[u]);
    }
    tmp_nodes.clear();

    // 更新属性
    route->attr->total_cost_dist = total_cost_dist;
    route->attr->total_cost_time = total_cost_time;

    // 推导时间窗
    infer_time_windows(begin_route_ind - 1, route);
}
