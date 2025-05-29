#include <unordered_set>
#include <unordered_map>
#include <tuple>
#include <climits>
#include <cfloat>

#include <spdlog/spdlog.h>

#include "tsp_construct.h"
#include "tsp_solution_node.h"

std::unique_ptr<TspSolutionRoute> TspConstructGreedy::to_construct(TspSolRouteOperator *tsp_operator)
{
    spdlog::info("start construct");
    TspContext *&tsp_context = tsp_operator->tsp_context;
    // 待访问的node索引集合，不考虑虚拟站点
    std::unordered_map<int, std::unordered_set<int>> priority_2_option_node_indices;
    for (int loc_ind = 0; loc_ind < tsp_context->locations.size() - 1; loc_ind++)
    {
        priority_2_option_node_indices[tsp_context->locations[loc_ind]->priority].insert(loc_ind);
    }

    std::unique_ptr<TspSolutionRoute> tsp_route = std::make_unique<TspSolutionRoute>(tsp_context->visit_loc_count);
    // 设置始发站点
    tsp_route->nodes[0] = std::make_unique<TspSolutionNode>(tsp_context->start_loc);
    for (TimeWindow *tw : tsp_context->start_loc->time_windows)
    {
        tsp_route->nodes[0]->time_windows.push_back(std::make_unique<TimeWindowPlan>(tw, tsp_context->start_loc->work_time));
    }
    // 设置结束站点
    tsp_route->nodes[tsp_context->visit_loc_count - 1] = std::make_unique<TspSolutionNode>(tsp_context->last_loc);
    // 迭代
    int cur_route_ind = 1;
    int waiting_loc_count = tsp_context->visit_loc_count - 2;
    while (waiting_loc_count > 0)
    {
        int cur_priority = tsp_context->ind_2_priority_map[cur_route_ind];

        std::unordered_set<int> option_node_indices = priority_2_option_node_indices[cur_priority];

        int begin_loc_ind, end_loc_ind;
        std::tie(begin_loc_ind, end_loc_ind) = tsp_context->priority_2_ind_range_map.at(cur_priority);

        waiting_loc_count -= end_loc_ind - begin_loc_ind + 1;

        // 分组标识
        std::string pre_group_mark = DEFAULT_GROUP_MARK;                                // 上一个站点的分组标识
        int pre_group_priority = INT_MIN;                                               // 上一个站点的分组优先级
        int pre_group_loc_count = 0;                                                    // 分组对应的站点数量
        bool constraint_enable_group = tsp_context->tsp_param->constraint_enable_group; // 启用分组约束

        for (int u = 0; u < end_loc_ind - begin_loc_ind + 1; ++u)
        {
            double min_val = DBL_MAX;
            double min_node_ind = DBL_MAX;
            std::string min_group_mark = DEFAULT_GROUP_MARK;
            int min_group_priority = INT_MAX;
            for (int cur_node_ind : option_node_indices)
            {
                if (cur_node_ind > end_loc_ind)
                {
                    continue;
                }
                // 判断分组标识是否继续插入
                if (constraint_enable_group && pre_group_mark != DEFAULT_GROUP_MARK && pre_group_mark != tsp_context->locations[cur_node_ind]->group_mark)
                {
                    continue;
                }
                tsp_operator->insert_node(cur_route_ind, tsp_context->locations[cur_node_ind], tsp_route);
                // delete tmp node
                tsp_route->attr->total_cost_dist -= tsp_route->nodes[cur_route_ind]->cost_dist;
                tsp_route->attr->total_cost_time -= tsp_route->nodes[cur_route_ind]->cost_time;
                tsp_route->nodes[cur_route_ind].reset();

                bool insert_opt_flag = false;
                // group mark为默认值时
                if (constraint_enable_group && pre_group_mark == DEFAULT_GROUP_MARK && min_group_mark == tsp_context->locations[cur_node_ind]->group_mark)
                {
                    // 优先考虑低优先级的站点
                    if (tsp_context->locations[cur_node_ind]->group_priority < min_group_priority)
                    {
                        insert_opt_flag = true;
                    }
                    // 优先级相同，看比较成本
                    else if (tsp_context->locations[cur_node_ind]->group_priority == min_group_priority)
                    {
                        insert_opt_flag = tsp_route->attr->obj_val < min_val;
                    }
                }
                else
                {
                    insert_opt_flag = tsp_route->attr->obj_val < min_val;
                }

                if (insert_opt_flag)
                {
                    min_val = tsp_route->attr->obj_val;
                    min_node_ind = cur_node_ind;
                    min_group_mark = tsp_context->locations[cur_node_ind]->group_mark;
                    min_group_priority = tsp_context->locations[cur_node_ind]->group_priority;
                }
            }

            tsp_operator->insert_node(cur_route_ind, tsp_context->locations[min_node_ind], tsp_route);
            option_node_indices.erase(min_node_ind);
            ++cur_route_ind;

            if (constraint_enable_group)
            {
                if (min_group_mark == DEFAULT_GROUP_MARK)
                {
                    pre_group_mark = DEFAULT_GROUP_MARK;
                    pre_group_priority = INT_MIN;
                    pre_group_loc_count = 0;
                }
                else
                {
                    pre_group_loc_count = pre_group_loc_count > 0 ? pre_group_loc_count : tsp_context->priority_2_group_mark_2_loc_count_map[cur_priority][min_group_mark];
                    pre_group_loc_count--;
                    if (pre_group_loc_count == 0)
                    {
                        pre_group_mark = DEFAULT_GROUP_MARK;
                        pre_group_priority = INT_MIN;
                    }
                    else
                    {
                        pre_group_mark = tsp_context->locations[min_node_ind]->group_mark;
                        pre_group_priority = tsp_context->locations[min_node_ind]->group_priority;
                    }
                }
            }
        }
    }
    spdlog::info("end of construct, sol obj = {}", tsp_route->attr->obj_val);
    return std::move(tsp_route);
}
