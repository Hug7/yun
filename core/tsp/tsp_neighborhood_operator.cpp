#include <algorithm>

#include <spdlog/spdlog.h>

#include "tsp_neighborhood_operator.h"

std::vector<std::unique_ptr<CandidateSol>> generate_candidate_sol(
    TspSolRouteOperator *&tsp_operator,
    std::unordered_map<NeighborhoodOperatorCode, std::unique_ptr<NeighborhoodOperator>> &neighborhood_operators,
    NeighborhoodOperatorCode operator_code, std::unique_ptr<TspSolutionRoute> &route)
{
    std::vector<std::unique_ptr<CandidateSol>> candidate_sols = neighborhood_operators.at(operator_code)->do_search(tsp_operator, route);

    if (candidate_sols.size() == 0)
    {
        return {};
    }

    std::sort(candidate_sols.begin(), candidate_sols.end(), [](std::unique_ptr<CandidateSol> &a, std::unique_ptr<CandidateSol> &b)
              { return a < b; });

    return std::move(candidate_sols);
}

std::unique_ptr<CandidateSol> generate_best_candidate_sol(
    TspSolRouteOperator *&tsp_operator,
    std::unordered_map<NeighborhoodOperatorCode, std::unique_ptr<NeighborhoodOperator>> &neighborhood_operators,
    NeighborhoodOperatorCode operator_code,
    std::unique_ptr<TspSolutionRoute> &route)
{
    std::vector<std::unique_ptr<CandidateSol>> candidate_sols = neighborhood_operators.at(operator_code)->do_search(tsp_operator, route);
    
    if (candidate_sols.size() == 0)
    {
        return nullptr;
    }

    int best_candidate_sol_ind = 0;
    int best_candidate_sol_obj = candidate_sols[0]->total_val;

    for (int u = 1; u < candidate_sols.size(); u++)
    {
        if (candidate_sols[u]->total_val < best_candidate_sol_obj)
        {
            best_candidate_sol_obj = candidate_sols[u]->total_val;
            best_candidate_sol_ind = u;
        }
    }

    return std::move(candidate_sols[best_candidate_sol_ind]);
}
