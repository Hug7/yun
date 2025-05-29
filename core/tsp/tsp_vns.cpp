#include <vector>
#include <algorithm>

#include "tsp_solution_check.h"
#include "tsp_vns.h"

std::unique_ptr<TspSolutionRoute> TspVns::do_search(
    TspSolRouteOperator *&tsp_operator,
    std::unordered_map<NeighborhoodOperatorCode, std::unique_ptr<NeighborhoodOperator>> &neighborhood_operators,
    std::unique_ptr<TspSolutionRoute> best_route)
{
    int iter = 0;
    int max_iter = 5;
    int last_change_iter = 0;
    int perturbation_time = 0;
    int max_perturbation_time = 5;
    std::unique_ptr<TspSolutionRoute> cur_route = best_route->deep_copy();

    std::vector<NeighborhoodOperatorCode> neighborhood_operator_codes_seq = {
        // NeighborhoodOperatorCode::ONE_SHIFT,
        NeighborhoodOperatorCode::SWAP,
        NeighborhoodOperatorCode::OPT2,
        NeighborhoodOperatorCode::OPT3,
    };
    int neighborhood_operator_count = neighborhood_operator_codes_seq.size();

    // do_check(tsp_operator, cur_route);

    while (iter++ < max_iter)
    {
        spdlog::debug("--------------------- iter = {} ---------------------", iter);
        int operator_code_seq_ind = -1; // 算子索引
        while (true)
        {
            ++operator_code_seq_ind;
            if (operator_code_seq_ind >= neighborhood_operator_count)
            {
                break;
            }
            // std::unique_ptr<CandidateSol> best_candidate_sol = generate_best_candidate_sol(tsp_operator, neighborhood_operators, neighborhood_operator_codes_seq[operator_code_seq_ind], cur_route);
            std::unique_ptr<CandidateSol> best_candidate_sol = neighborhood_operators.at(neighborhood_operator_codes_seq[operator_code_seq_ind])->search_best_sol(tsp_operator, cur_route);
            if (best_candidate_sol == nullptr)
            {
                continue;
            }
            spdlog::debug("iter = {}, operator code = {}, best candidate sol obj = {:.2f}; cur route obj = {:.2f}, best route obj = {:.2f}",
                          iter, operator_name(neighborhood_operator_codes_seq[operator_code_seq_ind]), best_candidate_sol->total_val, cur_route->attr->obj_val, best_route->attr->obj_val);
            // 判断是否更新当前解，如果没有改进
            if (best_candidate_sol->total_val >= cur_route->attr->obj_val)
            {
                continue;
            }
            // 执行邻域动作
            neighborhood_operators[neighborhood_operator_codes_seq[operator_code_seq_ind]]->implement_route_action(tsp_operator, best_candidate_sol->neighborhood_surface_action, cur_route);
            // do_check(tsp_operator, cur_route);

            operator_code_seq_ind = -1; // 重置算子
            // 判断是否更新最优解
            if (cur_route->attr->obj_val < best_route->attr->obj_val)
            {
                best_route = cur_route->deep_copy();
                iter = 1;
                perturbation_time = 0;
            }
        }
        // 扰动当前解
        cur_route = best_route->deep_copy();
        perturbation_time++;
        perturbation_time = std::min(perturbation_time, max_perturbation_time);
        for (int u = 0; u < perturbation_time; u++)
        {
            perturbation(tsp_operator, neighborhood_operators, cur_route);
        }
    }
    spdlog::info("route search in tsp vns, obj={:.2f}", best_route->attr->obj_val);
    return std::move(best_route);
}

void TspVns::perturbation(
    TspSolRouteOperator *&tsp_operator,
    std::unordered_map<NeighborhoodOperatorCode, std::unique_ptr<NeighborhoodOperator>> &neighborhood_operators,
    std::unique_ptr<TspSolutionRoute> &route)
{
    std::vector<int> prioritiy_list = tsp_operator->tsp_context->prioritiy_list;
    tsp_operator->tsp_context->random_utils->shuffle(prioritiy_list);
    for (int priority : prioritiy_list)
    {
        int route_ind_l, route_ind_r;
        std::tie(route_ind_l, route_ind_r) = tsp_operator->tsp_context->priority_2_ind_range_map[priority];
        if (route_ind_l == route_ind_r)
        {
            continue;
        }
        std::vector<int> route_indices;
        for (int u = route_ind_l; u <= route_ind_r; u++)
        {
            route_indices.push_back(u);
        }
        for (int u = 0; u < 10; u++)
        {
            tsp_operator->tsp_context->random_utils->shuffle(route_indices);
            if (std::abs(route_indices[0] - route_indices[1]) > 1)
            {
                // 判断是否满足约束
                if (tsp_operator->tsp_constraint_hard_utils->checkMoveAction(route_indices[0], route_indices[1], route, tsp_operator->tsp_context))
                {
                    break;
                }
            }
        }

        // 判断是否满足约束
        if (!tsp_operator->tsp_constraint_hard_utils->checkMoveAction(route_indices[0], route_indices[1], route, tsp_operator->tsp_context))
        {
            continue;;
        }

        std::vector<std::vector<int>> neighborhood_surface_action = {{route_indices[0], route_indices[1]}};
        // 执行邻域动作
        neighborhood_operators[NeighborhoodOperatorCode::ONE_SHIFT]->implement_route_action(tsp_operator, neighborhood_surface_action, route);
        return;
    }
    spdlog::warn("one shift perturbation is invaild in tsp vns!");
}
