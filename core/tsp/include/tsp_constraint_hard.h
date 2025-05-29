#ifndef TSP_CONSTRAINT_HARD_H
#define TSP_CONSTRAINT_HARD_H

#include <string>
#include <vector>
#include <unordered_set>
#include <memory>

#include "order.h"
#include "tsp_context.h"
#include "tsp_solution_route.h"

class TspConstraintHardInterface
{
public:
    TspConstraintHardInterface() {}
    virtual bool checkMoveAction(int route_ind_a, int route_ind_b, std::unique_ptr<TspSolutionRoute> &route, TspContext *&tsp_context) = 0;
    virtual bool checkSwapAction(int route_ind_a, int route_ind_b, std::unique_ptr<TspSolutionRoute> &route, TspContext *&tsp_context) = 0;
    virtual bool checkKoptAction(int begin_route_ind, int end_route_ind, std::vector<std::tuple<int, int>> &part_route_indices, std::unique_ptr<TspSolutionRoute> &route, TspContext *&tsp_context) = 0;
    virtual ~TspConstraintHardInterface() = default;
};

class TspConstraintHardGroup : public TspConstraintHardInterface
{
public:
    TspConstraintHardGroup() {}

    bool checkMoveAction(int route_ind_a, int route_ind_b, std::unique_ptr<TspSolutionRoute> &route, TspContext *&tsp_context) override;
    bool checkSwapAction(int route_ind_a, int route_ind_b, std::unique_ptr<TspSolutionRoute> &route, TspContext *&tsp_context) override;
    bool checkKoptAction(int begin_route_ind, int end_route_ind, std::vector<std::tuple<int, int>> &part_route_indices, std::unique_ptr<TspSolutionRoute> &route, TspContext *&tsp_context) override;
};

class TspConstraintHardUtils
{
private:
    std::vector<std::unique_ptr<TspConstraintHardInterface>> constraints;

public:
    TspConstraintHardUtils(std::vector<std::unique_ptr<TspConstraintHardInterface>> constraints);

    bool checkMoveAction(int route_ind_a, int route_ind_b, std::unique_ptr<TspSolutionRoute> &route, TspContext *&tsp_context);
    bool checkSwapAction(int route_ind_a, int route_ind_b, std::unique_ptr<TspSolutionRoute> &route, TspContext *&tsp_context);
    bool checkKoptAction(int begin_route_ind, int end_route_ind, std::vector<std::tuple<int, int>> &part_route_indices, std::unique_ptr<TspSolutionRoute> &route, TspContext *&tsp_context);
};

std::unique_ptr<TspConstraintHardUtils> create_tsp_constraint_hard_utils(TspContext *&tsp_context);

#endif // TSP_CONSTRAINT_HARD_H
