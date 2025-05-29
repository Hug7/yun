#include "tsp_solve.h"
#include "tsp_solution_route_operator.h"
#include "tsp_construct.h"
#include "tsp_neighborhood_operator.h"
#include "tsp_vns.h"

std::unique_ptr<TspSolutionRoute> fast_solve(TspContext *&tsp_context,
                                             std::unique_ptr<TspSolutionObjUtil> tsp_sol_obj_util,
                                             std::unique_ptr<TspConstraintHardUtils> tsp_constraint_hard_utils)
{
    TspSolRouteOperator *tsp_operator = new TspSolRouteOperator(tsp_context, std::move(tsp_sol_obj_util), std::move(tsp_constraint_hard_utils));
    // 构造
    TspConstructInterface *tsp_construct = new TspConstructGreedy();
    std::unique_ptr<TspSolutionRoute> best_sol = tsp_construct->to_construct(tsp_operator);
    delete tsp_construct;
    // 初始化算子集合
    std::unordered_map<NeighborhoodOperatorCode, std::unique_ptr<NeighborhoodOperator>> neighborhood_operators;
    neighborhood_operators[NeighborhoodOperatorCode::ONE_SHIFT] = std::make_unique<OneShiftOperatorImpl>();
    neighborhood_operators[NeighborhoodOperatorCode::SWAP] = std::make_unique<SwapOperatorImpl>();
    neighborhood_operators[NeighborhoodOperatorCode::OPT2] = std::make_unique<Opt2OperatorImpl>();
    neighborhood_operators[NeighborhoodOperatorCode::OPT3] = std::make_unique<Opt3OperatorImpl>();
    // VNS
    best_sol = TspVns::do_search(tsp_operator, neighborhood_operators, std::move(best_sol));
    // Tabu

    // 解析 todo 暂时不解析

    // 释放
    delete tsp_operator;
    return std::move(best_sol);
}
