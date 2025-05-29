#include <cmath>
#include <vector>

#include "tsp_solution_route.h"

/**
 * tsp solution route
 */

TspSolutionRoute::TspSolutionRoute(int loc_count)
{
    this->loc_count = loc_count;
    this->nodes = std::vector<std::unique_ptr<TspSolutionNode>>(loc_count);
    this->attr = std::make_unique<TspSolutionRouteAttr>(loc_count);
}

TspSolutionRoute::~TspSolutionRoute()
{
    this->nodes.clear();
}

std::unique_ptr<TspSolutionRoute> TspSolutionRoute::deep_copy()
{
    std::unique_ptr<TspSolutionRoute> tsp_sol_route = std::make_unique<TspSolutionRoute>(this->loc_count);
    int ind = 0;
    for (std::unique_ptr<TspSolutionNode> &cur_node : this->nodes)
    {
        tsp_sol_route->nodes[ind++] = cur_node->deep_copy();
    }
    tsp_sol_route->attr = this->attr->deep_copy();

    return std::move(tsp_sol_route);
}

TspSolutionNode *TspSolutionRoute::first_node()
{
    return this->nodes[0].get();
}

TspSolutionNode *TspSolutionRoute::last_node()
{
    return this->nodes[this->loc_count - 1].get();
}
