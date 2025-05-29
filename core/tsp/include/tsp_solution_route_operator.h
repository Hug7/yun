#ifndef TSP_SOLUTION_ROUTE_OPERATOR_H
#define TSP_SOLUTION_ROUTE_OPERATOR_H

#include <vector>
#include <tuple>
#include <memory>

#include <spdlog/spdlog.h>

#include "tsp_context.h"
#include "tsp_solution_obj.h"
#include "tsp_solution_route.h"
#include "tsp_constraint_hard.h"

class TspSolRouteOperator
{
public:
    TspContext *tsp_context;
    std::unique_ptr<TspSolutionObjUtil> tsp_sol_obj_util;
    std::unique_ptr<TspConstraintHardUtils> tsp_constraint_hard_utils;

    TspSolRouteOperator(TspContext *&tsp_context, std::unique_ptr<TspSolutionObjUtil> tsp_sol_obj_util, std::unique_ptr<TspConstraintHardUtils> tsp_constraint_hard_utils);

    ~TspSolRouteOperator();

    void insert_node(int route_ind, TspLocation *tsp_loc, std::unique_ptr<TspSolutionRoute> &route);

    void temp_infer_time_windows(int pre_begin_route_ind, int next_end_route_ind, std::vector<std::tuple<int, int>> &wait_upate_route_indices, std::unique_ptr<TspSolutionRouteAttr> &route_attr, std::unique_ptr<TspSolutionRoute> &route);
    void infer_time_windows(int pre_begin_route_ind, std::unique_ptr<TspSolutionRoute> &route);

    void temp_move(int route_ind_a, int route_ind_b, std::unique_ptr<TspSolutionRouteAttr> &route_attr, std::unique_ptr<TspSolutionRoute> &route);
    void move(int route_ind_a, int route_ind_b, std::unique_ptr<TspSolutionRoute> &route);

    void temp_swap(int route_ind_a, int route_ind_b, std::unique_ptr<TspSolutionRouteAttr> &route_attr, std::unique_ptr<TspSolutionRoute> &route);
    void swap(int route_ind_a, int route_ind_b, std::unique_ptr<TspSolutionRoute> &route);

    void temp_k_opt(int begin_route_ind, int end_route_ind, std::vector<std::tuple<int, int>> &part_route_indices, std::unique_ptr<TspSolutionRouteAttr> &route_attr, std::unique_ptr<TspSolutionRoute> &route);
    void k_opt(std::vector<std::vector<int>> &part_route_indices, std::unique_ptr<TspSolutionRoute> &route);
};

#endif // TSP_SOLUTION_ROUTE_OPERATOR_H
