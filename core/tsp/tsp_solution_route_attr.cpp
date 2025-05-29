#include "tsp_solution_route_attr.h"

/**
 * tsp solution route attr
 */

 TspSolutionRouteAttr::TspSolutionRouteAttr(int loc_count)
 {
     this->obj_val = 0.;
     this->total_cost_dist = 0;
     this->total_cost_time = 0;
     this->wait_times = std::vector<long>(loc_count);
     this->over_times = std::vector<long>(loc_count);
     this->penalty_val = 0.;
 }
 
 TspSolutionRouteAttr::TspSolutionRouteAttr(double obj_val, long total_cost_dist, long total_cost_time, std::vector<long> wait_times, std::vector<long> over_times, double penalty_val)
 {
     this->obj_val = obj_val;
     this->total_cost_dist = total_cost_dist;
     this->total_cost_time = total_cost_time;
     this->wait_times = wait_times;
     this->over_times = over_times;
     this->penalty_val = penalty_val;
 }
 
 TspSolutionRouteAttr::TspSolutionRouteAttr(std::unique_ptr<TspSolutionRouteAttr> &tsp_sol_route_attr)
 {
     this->obj_val = tsp_sol_route_attr->obj_val;
     this->total_cost_dist = tsp_sol_route_attr->total_cost_dist;
     this->total_cost_time = tsp_sol_route_attr->total_cost_time;
     this->wait_times = std::vector<long>(tsp_sol_route_attr->wait_times);
     this->over_times = std::vector<long>(tsp_sol_route_attr->over_times);
     this->penalty_val = tsp_sol_route_attr->penalty_val;
 }
 
 std::unique_ptr<TspSolutionRouteAttr> TspSolutionRouteAttr::deep_copy()
 {
     std::unique_ptr<TspSolutionRouteAttr> route_attr = std::make_unique<TspSolutionRouteAttr>(this->wait_times.size());
     route_attr->obj_val = this->obj_val;
     route_attr->total_cost_dist = this->total_cost_dist;
     route_attr->total_cost_time = this->total_cost_time;
     route_attr->wait_times = std::vector<long>(this->wait_times);
     route_attr->over_times = std::vector<long>(this->over_times);
     route_attr->penalty_val = this->penalty_val;
     return std::move(route_attr);
 }
 