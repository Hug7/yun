#include <cfloat>

#include "tsp_neighborhood_operator.h"

std::unique_ptr<CandidateSol> Opt2OperatorImpl::do_2opt(int pick_ind_a, int pick_ind_b, TspSolRouteOperator *tsp_operator, std::unique_ptr<TspSolutionRoute> &route)
{
    // 邻域动作
    std::vector<std::tuple<int, int>> part_route_indices = {{pick_ind_b, pick_ind_a}};
    // 判断是否满足约束
    if (!tsp_operator->tsp_constraint_hard_utils->checkKoptAction(pick_ind_a, pick_ind_b, part_route_indices, route, tsp_operator->tsp_context))
    {
        return nullptr;
    }
    // copy route属性
    std::unique_ptr<TspSolutionRouteAttr> route_attr = std::make_unique<TspSolutionRouteAttr>(route->attr);
    // 邻域移动
    tsp_operator->temp_k_opt(pick_ind_a, pick_ind_b, part_route_indices, route_attr, route);
    // 计算目标值
    tsp_operator->tsp_sol_obj_util->compute(tsp_operator->tsp_context, route_attr);
    // 记录候选解
    std::unique_ptr<CandidateSol> candidate_sol = std::make_unique<CandidateSol>();
    // 邻域动作
    if (pick_ind_a < pick_ind_b)
    {
        for (int u = pick_ind_b; u >= pick_ind_a; --u)
        {
            candidate_sol->neighborhood_action.push_back(route->nodes[u]->loc_ind);
        }
    }
    else
    {
        for (int u = pick_ind_b; u <= pick_ind_a; ++u)
        {
            candidate_sol->neighborhood_action.push_back(route->nodes[u]->loc_ind);
        }
    }
    candidate_sol->dist = route_attr->total_cost_dist;
    // 记录邻域表层动作
    candidate_sol->neighborhood_surface_action.push_back({pick_ind_a, pick_ind_b});
    candidate_sol->neighborhood_surface_action.push_back({pick_ind_b, pick_ind_a});
    // 记录邻域方法名称
    candidate_sol->neighborhood_operator = this->name;
    // 记录目标值
    candidate_sol->obj_val = route_attr->obj_val;
    // 记录惩罚值
    candidate_sol->punishment_val = route_attr->penalty_val;
    // 记录总的目标值
    candidate_sol->total_val = candidate_sol->obj_val + candidate_sol->punishment_val;

    return std::move(candidate_sol);
}

std::vector<std::unique_ptr<CandidateSol>> Opt2OperatorImpl::do_search(TspSolRouteOperator *&tsp_operator, std::unique_ptr<TspSolutionRoute> &route)
{
    TspContext *&tsp_context = tsp_operator->tsp_context;

    std::vector<std::unique_ptr<CandidateSol>> candidate_sols;
    int start_ind, end_ind;
    for (auto index_range : tsp_context->priority_2_ind_range_map)
    {
        std::tie(start_ind, end_ind) = index_range.second;
        for (int u = start_ind; u < end_ind; ++u)
        {
            for (int m = u + 2; m <= end_ind; ++m)
            {
                std::unique_ptr<CandidateSol> tmp_candidate_sol = this->do_2opt(u, m, tsp_operator, route);
                if (tmp_candidate_sol != nullptr)
                {
                    candidate_sols.emplace_back(std::move(tmp_candidate_sol));
                }
            }
        }
    }
    return candidate_sols;
}

std::unique_ptr<CandidateSol> Opt2OperatorImpl::search_best_sol(TspSolRouteOperator *&tsp_operator, std::unique_ptr<TspSolutionRoute> &route)
{
    TspContext *&tsp_context = tsp_operator->tsp_context;

    std::unique_ptr<CandidateSol> best_candidate_sol;
    double best_candidate_sol_total_val = DBL_MAX;
    int start_ind, end_ind;
    for (auto index_range : tsp_context->priority_2_ind_range_map)
    {
        std::tie(start_ind, end_ind) = index_range.second;
        for (int u = start_ind; u < end_ind; ++u)
        {
            for (int m = u + 2; m <= end_ind; ++m)
            {
                std::unique_ptr<CandidateSol> tmp_candidate_sol = this->do_2opt(u, m, tsp_operator, route);
                if (tmp_candidate_sol != nullptr && tmp_candidate_sol->total_val < best_candidate_sol_total_val)
                {
                    best_candidate_sol_total_val = tmp_candidate_sol->total_val;
                    best_candidate_sol = std::move(tmp_candidate_sol);
                }
            }
        }
    }
    return std::move(best_candidate_sol);
}

void Opt2OperatorImpl::implement_route_action(TspSolRouteOperator *&tsp_operator, std::vector<std::vector<int>> &neighborhood_surface_action, std::unique_ptr<TspSolutionRoute> &route)
{
    // 邻域移动
    tsp_operator->k_opt(neighborhood_surface_action, route);
    // 更新目标函数值
    tsp_operator->tsp_sol_obj_util->compute(tsp_operator->tsp_context, route->attr);
}

int Opt2OperatorImpl::produce_max_neighbor(TspSolRouteOperator *tsp_operator, std::unique_ptr<TspSolutionRoute> &route)
{
    int capacity = 0;
    int start_ind, end_ind;
    for (auto index_range : tsp_operator->tsp_context->priority_2_ind_range_map)
    {
        std::tie(start_ind, end_ind) = index_range.second;
        capacity += (end_ind - start_ind + 1) / 2 * ((end_ind - start_ind) + 0); // n(a_n + a_1)/2
    }
    return std::max(capacity, 10);
}
