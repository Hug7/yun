#include <vector>
#include <tuple>

#include <spdlog/spdlog.h>

#include "tsp_solution_route_operator.h"


void TspSolRouteOperator::temp_move(int route_ind_a, int route_ind_b, std::unique_ptr<TspSolutionRouteAttr> &route_attr, std::unique_ptr<TspSolutionRoute> &ori_route)
{
    /**
     * if a - b <= 1
     *   do swap
     * if a > b
     *   b-1,b,b+1,...,a-1,a,a+1 -> b-1,a,b,b+1,...,a-1,a+1
     *   b-1,b,b+1(a-1),a,a+1 -> b-1,a,b,b+1(a-1),a+1
     * if a < b
     *   a-1,a,a+1,...,b-1,b,b+1 -> a-1,a+1,...,b-1,b,a,b+1
     *   a-1,a,a+1(b-1),b,b+1 -> a-1,a+1(b-1),b,a,b+1
     */
    if (std::abs(route_ind_a - route_ind_b) <= 1)
    {
        temp_swap(route_ind_a, route_ind_b, route_attr, ori_route);
        return;
    }
    std::vector<std::unique_ptr<TspSolutionNode>> &nodes = ori_route->nodes;
    // 拆了选中节点的信息
    long total_cost_dist = ori_route->attr->total_cost_dist;
    long total_cost_time = ori_route->attr->total_cost_time;

    total_cost_dist -= nodes[route_ind_a]->cost_dist;
    total_cost_dist -= nodes[route_ind_a + 1]->cost_dist;

    total_cost_dist += this->tsp_context->cost_matrix->get_dist(nodes[route_ind_a - 1]->matrix_ind, nodes[route_ind_a + 1]->matrix_ind);

    total_cost_time -= nodes[route_ind_a]->cost_time;
    total_cost_time -= nodes[route_ind_a + 1]->cost_time;
    total_cost_time += this->tsp_context->cost_matrix->get_time(nodes[route_ind_a - 1]->matrix_ind, nodes[route_ind_a + 1]->matrix_ind);
    int pre_begin_route_ind;
    std::vector<std::tuple<int, int>> wait_upate_route_indices;
    if (route_ind_a > route_ind_b)
    {
        // 断开插入目标节点的信息
        total_cost_dist -= nodes[route_ind_b]->cost_dist;
        total_cost_time -= nodes[route_ind_b]->cost_time;
        // 将选中的节点插入到目标位置
        total_cost_dist += this->tsp_context->cost_matrix->get_dist(nodes[route_ind_b - 1]->matrix_ind, nodes[route_ind_a]->matrix_ind);
        total_cost_dist += this->tsp_context->cost_matrix->get_dist(nodes[route_ind_a]->matrix_ind, nodes[route_ind_b]->matrix_ind);
        
        total_cost_time += this->tsp_context->cost_matrix->get_time(nodes[route_ind_b - 1]->matrix_ind, nodes[route_ind_a]->matrix_ind);
        total_cost_time += this->tsp_context->cost_matrix->get_time(nodes[route_ind_a]->matrix_ind, nodes[route_ind_b]->matrix_ind);
        // 记录待更新时间窗的节点信息
        pre_begin_route_ind = route_ind_b - 1;
        wait_upate_route_indices = {
            std::make_tuple(route_ind_a, route_ind_a),
            std::make_tuple(route_ind_b, route_ind_a - 1), 
            std::make_tuple(route_ind_a + 1, ori_route->loc_count - 1)
        };
    } else
    {
        // 断开插入目标节点的信息
        total_cost_dist -= nodes[route_ind_b + 1]->cost_dist;
        total_cost_time -= nodes[route_ind_b + 1]->cost_time;
        // 将选中的节点插入到目标位置
        total_cost_dist += this->tsp_context->cost_matrix->get_dist(nodes[route_ind_a]->matrix_ind, nodes[route_ind_b + 1]->matrix_ind);
        total_cost_dist += this->tsp_context->cost_matrix->get_dist(nodes[route_ind_b]->matrix_ind, nodes[route_ind_a]->matrix_ind);
        
        total_cost_time += this->tsp_context->cost_matrix->get_time(nodes[route_ind_a]->matrix_ind, nodes[route_ind_b + 1]->matrix_ind);
        total_cost_time += this->tsp_context->cost_matrix->get_time(nodes[route_ind_b]->matrix_ind, nodes[route_ind_a]->matrix_ind);
        // 记录待更新时间窗的节点信息
        pre_begin_route_ind = route_ind_a - 1;
        wait_upate_route_indices = {
            std::make_tuple(route_ind_a + 1, route_ind_b), 
            std::make_tuple(route_ind_a, route_ind_a), 
            std::make_tuple(route_ind_b + 1, ori_route->loc_count - 1)
        };
    }
    // 更新属性
    route_attr->total_cost_dist = total_cost_dist;
    route_attr->total_cost_time = total_cost_time;
    // 推导时间窗
    temp_infer_time_windows(pre_begin_route_ind, ori_route->loc_count, wait_upate_route_indices, route_attr, ori_route);

}


void TspSolRouteOperator::move(int route_ind_a, int route_ind_b, std::unique_ptr<TspSolutionRoute> &ori_route)
{
    /**
     * if a - b <= 1
     *   do swap
     * if a > b
     *   b-1,b,b+1,...,a-1,a,a+1 -> b-1,a,b,b+1,...,a-1,a+1
     *   b-1,b,b+1(a-1),a,a+1 -> b-1,a,b,b+1(a-1),a+1
     * if a < b
     *   a-1,a,a+1,...,b-1,b,b+1 -> a-1,a+1,...,b-1,b,a,b+1
     *   a-1,a,a+1(b-1),b,b+1 -> a-1,a+1(b-1),b,a,b+1
     */
    if (std::abs(route_ind_a - route_ind_b) <= 1)
    {
        swap(route_ind_a, route_ind_b, ori_route);
        return;
    }
    std::vector<std::unique_ptr<TspSolutionNode>> &nodes = ori_route->nodes;
    // 拆了选中节点的信息
    int total_cost_dist = ori_route->attr->total_cost_dist;
    int total_cost_time = ori_route->attr->total_cost_time;

    total_cost_dist -= nodes[route_ind_a]->cost_dist;
    total_cost_dist -= nodes[route_ind_a + 1]->cost_dist;
    nodes[route_ind_a + 1]->cost_dist = this->tsp_context->cost_matrix->get_dist(nodes[route_ind_a - 1]->matrix_ind, nodes[route_ind_a + 1]->matrix_ind);
    total_cost_dist += nodes[route_ind_a + 1]->cost_dist;
    

    total_cost_time -= nodes[route_ind_a]->cost_time;
    total_cost_time -= nodes[route_ind_a + 1]->cost_time;
    nodes[route_ind_a + 1]->cost_time = this->tsp_context->cost_matrix->get_time(nodes[route_ind_a - 1]->matrix_ind, nodes[route_ind_a + 1]->matrix_ind);
    total_cost_time += nodes[route_ind_a + 1]->cost_time;
    int pre_begin_route_ind;
    std::vector<std::tuple<int, int>> wait_upate_route_indices;
    if (route_ind_a > route_ind_b)
    {
        // 断开插入目标节点的信息
        total_cost_dist -= nodes[route_ind_b]->cost_dist;
        total_cost_time -= nodes[route_ind_b]->cost_time;
        // 将选中的节点插入到目标位置
        nodes[route_ind_a]->cost_dist = this->tsp_context->cost_matrix->get_dist(nodes[route_ind_b - 1]->matrix_ind, nodes[route_ind_a]->matrix_ind);
        nodes[route_ind_b]->cost_dist = this->tsp_context->cost_matrix->get_dist(nodes[route_ind_a]->matrix_ind, nodes[route_ind_b]->matrix_ind);

        total_cost_dist += nodes[route_ind_a]->cost_dist + nodes[route_ind_b]->cost_dist;
        
        nodes[route_ind_a]->cost_time = this->tsp_context->cost_matrix->get_time(nodes[route_ind_b - 1]->matrix_ind, nodes[route_ind_a]->matrix_ind);
        nodes[route_ind_b]->cost_time = this->tsp_context->cost_matrix->get_time(nodes[route_ind_a]->matrix_ind, nodes[route_ind_b]->matrix_ind);

        total_cost_time += nodes[route_ind_a]->cost_time + nodes[route_ind_b]->cost_time;

        // 移动节点
        std::unique_ptr<TspSolutionNode> tmp_node = std::move(nodes[route_ind_a]);
        for (int u = route_ind_a; u > route_ind_b; u--)
        {
            nodes[u] = std::move(nodes[u - 1]);
        }
        nodes[route_ind_b] = std::move(tmp_node);

        // 记录待更新时间窗的节点信息
        pre_begin_route_ind = route_ind_b - 1;
    } else
    {
        // 断开插入目标节点的信息
        total_cost_dist -= nodes[route_ind_b + 1]->cost_dist;
        total_cost_time -= nodes[route_ind_b + 1]->cost_time;
        // 将选中的节点插入到目标位置
        nodes[route_ind_b + 1]->cost_dist = this->tsp_context->cost_matrix->get_dist(nodes[route_ind_a]->matrix_ind, nodes[route_ind_b + 1]->matrix_ind);
        nodes[route_ind_a]->cost_dist = this->tsp_context->cost_matrix->get_dist(nodes[route_ind_b]->matrix_ind, nodes[route_ind_a]->matrix_ind);

        total_cost_dist += nodes[route_ind_b + 1]->cost_dist + nodes[route_ind_a]->cost_dist;
        
        nodes[route_ind_b + 1]->cost_time = this->tsp_context->cost_matrix->get_time(nodes[route_ind_a]->matrix_ind, nodes[route_ind_b + 1]->matrix_ind);
        nodes[route_ind_a]->cost_time = this->tsp_context->cost_matrix->get_time(nodes[route_ind_b]->matrix_ind, nodes[route_ind_a]->matrix_ind);

        total_cost_time += nodes[route_ind_b + 1]->cost_time + nodes[route_ind_a]->cost_time;
        // 移动节点
        std::unique_ptr<TspSolutionNode> tmp_node = std::move(nodes[route_ind_a]);
        for (int u = route_ind_a; u < route_ind_b; u++)
        {
            nodes[u] = std::move(nodes[u + 1]);
        }
        nodes[route_ind_b] = std::move(tmp_node);
        // 记录待更新时间窗的节点信息
        pre_begin_route_ind = route_ind_a - 1;
    }
    
    // 更新属性
    ori_route->attr->total_cost_dist = total_cost_dist;
    ori_route->attr->total_cost_time = total_cost_time;
    // 推导时间窗
    infer_time_windows(pre_begin_route_ind, ori_route);

}
