#ifndef TSP_NEIGHBORHOOD_OPERATOR_H
#define TSP_NEIGHBORHOOD_OPERATOR_H

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <string_view>
#include <climits>
#include <cfloat>

#include <spdlog/spdlog.h>

#include "tsp_solution_route_operator.h"
#include "tsp_candidate_solution.h"

enum class NeighborhoodOperatorCode : int
{
    ONE_SHIFT = 0,
    SWAP = 1,
    OPT2 = 2,
    OPT3 = 3,
};

namespace std
{
    template <>
    struct hash<NeighborhoodOperatorCode>
    {
        std::size_t operator()(NeighborhoodOperatorCode code) const
        {
            return static_cast<std::size_t>(code);
        }
    };
}

namespace std
{
    template <>
    struct equal_to<NeighborhoodOperatorCode>
    {
        bool operator()(NeighborhoodOperatorCode lhs, NeighborhoodOperatorCode rhs) const
        {
            return lhs == rhs;
        }
    };
}

constexpr std::string_view operator_name(NeighborhoodOperatorCode operator_code)
{
    switch (operator_code) {
        case NeighborhoodOperatorCode::ONE_SHIFT:   return "ONE_SHIFT";
        case NeighborhoodOperatorCode::SWAP: return "SWAP";
        case NeighborhoodOperatorCode::OPT2:  return "OPT2";
        case NeighborhoodOperatorCode::OPT3:  return "OPT3";
        default:           return "Unknown";
    }
}


class NeighborhoodOperator
{
public:
    std::string name;
    NeighborhoodOperator(std::string name) : name(name) {};
    // 邻域搜索
    virtual std::vector<std::unique_ptr<CandidateSol>> do_search(TspSolRouteOperator *&tsp_operator, std::unique_ptr<TspSolutionRoute> &route) = 0;
    // 邻域搜索
    virtual std::unique_ptr<CandidateSol> search_best_sol(TspSolRouteOperator *&tsp_operator, std::unique_ptr<TspSolutionRoute> &route) = 0;
    // 应用邻域动作
    virtual void implement_route_action(TspSolRouteOperator *&tsp_operator, std::vector<std::vector<int>> &neighborhood_surface_action, std::unique_ptr<TspSolutionRoute> &route) = 0;
    // 生成最大邻域可能产生解的数量
    virtual int produce_max_neighbor(TspSolRouteOperator *tsp_operator, std::unique_ptr<TspSolutionRoute> &route) = 0;

    virtual ~NeighborhoodOperator() = default;
};

class OneShiftOperatorImpl : public NeighborhoodOperator
{
private:
    std::unique_ptr<CandidateSol> do_shift(int pick_ind_a, int pick_ind_b, TspSolRouteOperator *tsp_operator, std::unique_ptr<TspSolutionRoute> &route);

public:
    OneShiftOperatorImpl() : NeighborhoodOperator("OneShift") {};
    std::vector<std::unique_ptr<CandidateSol>> do_search(TspSolRouteOperator *&tsp_operator, std::unique_ptr<TspSolutionRoute> &route) override;
    std::unique_ptr<CandidateSol> search_best_sol(TspSolRouteOperator *&tsp_operator, std::unique_ptr<TspSolutionRoute> &route) override;
    void implement_route_action(TspSolRouteOperator *&tsp_operator, std::vector<std::vector<int>> &neighborhood_surface_action, std::unique_ptr<TspSolutionRoute> &route) override;
    int produce_max_neighbor(TspSolRouteOperator *tsp_operator, std::unique_ptr<TspSolutionRoute> &route) override;
};

class SwapOperatorImpl : public NeighborhoodOperator
{
private:
    std::unique_ptr<CandidateSol> do_swap(int pick_ind_a, int pick_ind_b, TspSolRouteOperator *tsp_operator, std::unique_ptr<TspSolutionRoute> &route);

public:
    SwapOperatorImpl() : NeighborhoodOperator("Swap") {};
    std::vector<std::unique_ptr<CandidateSol>> do_search(TspSolRouteOperator *&tsp_operator, std::unique_ptr<TspSolutionRoute> &route) override;
    std::unique_ptr<CandidateSol> search_best_sol(TspSolRouteOperator *&tsp_operator, std::unique_ptr<TspSolutionRoute> &route) override;
    void implement_route_action(TspSolRouteOperator *&tsp_operator, std::vector<std::vector<int>> &neighborhood_surface_action, std::unique_ptr<TspSolutionRoute> &route) override;
    int produce_max_neighbor(TspSolRouteOperator *tsp_operator, std::unique_ptr<TspSolutionRoute> &route) override;
};

class Opt2OperatorImpl : public NeighborhoodOperator
{
private:
    std::unique_ptr<CandidateSol> do_2opt(int pick_ind_a, int pick_ind_b, TspSolRouteOperator *tsp_operator, std::unique_ptr<TspSolutionRoute> &route);

public:
    Opt2OperatorImpl() : NeighborhoodOperator("Opt2") {};
    std::vector<std::unique_ptr<CandidateSol>> do_search(TspSolRouteOperator *&tsp_operator, std::unique_ptr<TspSolutionRoute> &route) override;
    std::unique_ptr<CandidateSol> search_best_sol(TspSolRouteOperator *&tsp_operator, std::unique_ptr<TspSolutionRoute> &route) override;
    void implement_route_action(TspSolRouteOperator *&tsp_operator, std::vector<std::vector<int>> &neighborhood_surface_action, std::unique_ptr<TspSolutionRoute> &route) override;
    int produce_max_neighbor(TspSolRouteOperator *tsp_operator, std::unique_ptr<TspSolutionRoute> &route) override;
};

class Opt3OperatorImpl : public NeighborhoodOperator
{
private:
    std::unique_ptr<CandidateSol> do_3opt(int pick_ind_a, int pick_ind_b, std::vector<std::tuple<int, int>> part_route_indices, TspSolRouteOperator *tsp_operator, std::unique_ptr<TspSolutionRoute> &route);

public:
    Opt3OperatorImpl() : NeighborhoodOperator("Opt3") {};
    std::vector<std::unique_ptr<CandidateSol>> do_search(TspSolRouteOperator *&tsp_operator, std::unique_ptr<TspSolutionRoute> &route) override;
    std::unique_ptr<CandidateSol> search_best_sol(TspSolRouteOperator *&tsp_operator, std::unique_ptr<TspSolutionRoute> &route) override;
    void implement_route_action(TspSolRouteOperator *&tsp_operator, std::vector<std::vector<int>> &neighborhood_surface_action, std::unique_ptr<TspSolutionRoute> &route) override;
    int produce_max_neighbor(TspSolRouteOperator *tsp_operator, std::unique_ptr<TspSolutionRoute> &route) override;
};


std::vector<std::unique_ptr<CandidateSol>> generate_candidate_sol(
    TspSolRouteOperator *&tsp_operator,
    std::unordered_map<NeighborhoodOperatorCode, std::unique_ptr<NeighborhoodOperator>> &neighborhood_operators,
    NeighborhoodOperatorCode operator_code,
    std::unique_ptr<TspSolutionRoute> &route);

std::unique_ptr<CandidateSol> generate_best_candidate_sol(
    TspSolRouteOperator *&tsp_operator,
    std::unordered_map<NeighborhoodOperatorCode, std::unique_ptr<NeighborhoodOperator>> &neighborhood_operators,
    NeighborhoodOperatorCode operator_code,
    std::unique_ptr<TspSolutionRoute> &route);

#endif // TSP_NEIGHBORHOOD_OPERATOR_H
