#ifndef TSP_SOLUTION_ROUTE_H
#define TSP_SOLUTION_ROUTE_H

#include <vector>
#include <memory>

#include "tsp_solution_node.h"
#include "tsp_solution_route_attr.h"

class TspSolutionRoute
{
public:
    int loc_count;
    std::vector<std::unique_ptr<TspSolutionNode>> nodes;
    std::unique_ptr<TspSolutionRouteAttr> attr;

    TspSolutionRoute(int loc_count);
    ~TspSolutionRoute();
    std::unique_ptr<TspSolutionRoute> deep_copy();
    TspSolutionNode *first_node();
    TspSolutionNode *last_node();
};

#endif // TSP_SOLUTION_ROUTE_H
