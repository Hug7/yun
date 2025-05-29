#include "tsp_constraint_hard.h"

TspConstraintHardUtils::TspConstraintHardUtils(std::vector<std::unique_ptr<TspConstraintHardInterface>> constraints)
{
    this->constraints = std::move(constraints);
}

bool TspConstraintHardUtils::checkMoveAction(int route_ind_a, int route_ind_b, std::unique_ptr<TspSolutionRoute> &route, TspContext *&tsp_context)
{
    for (auto &constr : this->constraints)
    {
        if (!constr->checkMoveAction(route_ind_a, route_ind_b, route, tsp_context))
        {
            return false;
        }
    }
    return true;
}

bool TspConstraintHardUtils::checkSwapAction(int route_ind_a, int route_ind_b, std::unique_ptr<TspSolutionRoute> &route, TspContext *&tsp_context)
{
    for (auto &constr : this->constraints)
    {
        if (!constr->checkSwapAction(route_ind_a, route_ind_b, route, tsp_context))
        {
            return false;
        }
    }
    return true;
}

bool TspConstraintHardUtils::checkKoptAction(int begin_route_ind, int end_route_ind, std::vector<std::tuple<int, int>> &part_route_indices, std::unique_ptr<TspSolutionRoute> &route, TspContext *&tsp_context)
{
    for (auto &constr : this->constraints)
    {
        if (!constr->checkKoptAction(begin_route_ind, end_route_ind, part_route_indices, route, tsp_context))
        {
            return false;
        }
    }
    return true;
}

std::unique_ptr<TspConstraintHardUtils> create_tsp_constraint_hard_utils(TspContext *&tsp_context)
{
    std::vector<std::unique_ptr<TspConstraintHardInterface>> constraints;

    if (tsp_context->tsp_param->constraint_enable_group)
    {
        constraints.push_back(std::make_unique<TspConstraintHardGroup>());
    }
    
    return std::make_unique<TspConstraintHardUtils>(std::move(constraints));
}
