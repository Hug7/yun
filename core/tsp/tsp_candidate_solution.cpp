#include "tsp_candidate_solution.h"


std::unique_ptr<CandidateSol> CandidateSol::copy()
{
    std::unique_ptr<CandidateSol> candidate_sol = std::make_unique<CandidateSol>();
    candidate_sol->neighborhood_action = std::vector<int>(this->neighborhood_action);
    candidate_sol->neighborhood_surface_action = std::vector<std::vector<int>>(this->neighborhood_surface_action);
    candidate_sol->neighborhood_operator = this->neighborhood_operator;
    candidate_sol->obj_val = this->obj_val;
    candidate_sol->punishment_val = this->punishment_val;
    candidate_sol->total_val = this->total_val;

    return std::move(candidate_sol);
}