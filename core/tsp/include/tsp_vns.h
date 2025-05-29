#ifndef TSP_VNS_H
#define TSP_VNS_H

#include <string>
#include <unordered_map>
#include <memory>

#include <spdlog/spdlog.h>

#include "tsp_solution_route_operator.h"
#include "tsp_neighborhood_operator.h"


namespace TspVns
{
    std::unique_ptr<TspSolutionRoute> do_search(TspSolRouteOperator *&tsp_operator, std::unordered_map<NeighborhoodOperatorCode, std::unique_ptr<NeighborhoodOperator>> &neighborhood_operators, std::unique_ptr<TspSolutionRoute> best_route);

    void perturbation(TspSolRouteOperator *&tsp_operator, std::unordered_map<NeighborhoodOperatorCode, std::unique_ptr<NeighborhoodOperator>> &neighborhood_operators, std::unique_ptr<TspSolutionRoute> &route);
}

#endif // TSP_VNS_H
