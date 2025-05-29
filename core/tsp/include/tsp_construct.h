#ifndef TSP_CONSTRUCT_H
#define TSP_CONSTRUCT_H

#include <memory>

#include "default_parameter.h"
#include "tsp_solution_route.h"
#include "tsp_solution_route_operator.h"

class TspConstructInterface
{

public:
    virtual ~TspConstructInterface() {}

    virtual std::unique_ptr<TspSolutionRoute> to_construct(TspSolRouteOperator *tsp_operator) = 0;
};

class TspConstructGreedy : public TspConstructInterface
{
public:
    std::unique_ptr<TspSolutionRoute> to_construct(TspSolRouteOperator *tsp_operator) override;
};


#endif // TSP_CONSTRUCT_H
