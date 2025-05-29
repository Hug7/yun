#ifndef TSP_SOLUTION_ROUTE_ATTR_H
#define TSP_SOLUTION_ROUTE_ATTR_H

#include <vector>
#include <memory>

class TspSolutionRouteAttr
{
public:
    double obj_val;  // 目标函数值
    long total_cost_dist;  // 总行驶距离
    long total_cost_time;  // 总行驶时间
    std::vector<long> wait_times;  // 等待时间
    std::vector<long> over_times;  // 超时时间
    double penalty_val;  // 罚函数值

    TspSolutionRouteAttr(int loc_count);
    TspSolutionRouteAttr(double obj_val, long total_cost_dist, long total_cost_time, std::vector<long> wait_times, std::vector<long> over_times, double penalty_val);
    TspSolutionRouteAttr(std::unique_ptr<TspSolutionRouteAttr> &tsp_sol_route_attr);
    std::unique_ptr<TspSolutionRouteAttr> deep_copy();
};

#endif // TSP_SOLUTION_ROUTE_ATTR_H