#ifndef TSP_SOLVE_H
#define TSP_SOLVE_H

#include <memory>

#include "tsp_context.h"
#include "tsp_solution_route.h"
#include "tsp_solution_obj.h"
#include "tsp_constraint_hard.h"

std::unique_ptr<TspSolutionRoute> fast_solve(TspContext *&tsp_context,
                                             std::unique_ptr<TspSolutionObjUtil> tsp_sol_obj_util,
                                             std::unique_ptr<TspConstraintHardUtils> tsp_constraint_hard_utils);

#endif // TSP_SOLVE_H
