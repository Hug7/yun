#ifndef TSP_MODEL_H
#define TSP_MODEL_H

#include <memory>
#include <unordered_map>

#include "rapidcsv.h"
#include <spdlog/spdlog.h>

#include "location.h"
#include "order.h"
#include "tsp_context.h"
#include "tsp_solution_obj.h"
#include "tsp_solution_route.h"
#include "tsp_solve.h"

class TspRoutePlanParam
{
public:
    std::vector<std::string> order_ids = std::vector<std::string>();
    int matrix_type = -1;

    ~TspRoutePlanParam() {}
};

class TspModel
{
private:
    std::vector<Location *> locations; // <location_id, Location>
    int loc_count;
    std::vector<Order *> orders; // <order_id, Order>
    int order_count;
    std::unordered_map<std::string, int> loc_id_to_ind_map;   // <location_id, location_ind>
    std::unordered_map<int, std::string> loc_ind_to_id_map;   // <location_ind, location_id>
    std::unordered_map<std::string, int> order_id_to_ind_map; // <order_id, order_ind>
    std::unordered_map<int, CostMatrix *> cost_matrixs;
    TspParam *tsp_param;
    Location *virtual_loc;

public:
    TspModel();
    ~TspModel();

    void set_locations(std::vector<Location *> &locations);
    void set_orders(std::vector<Order *> &orders);

    void load_file(std::string &file_path);

    Location *&get_location(const std::string &location_id);

    Order *&get_order(const std::string &order_id);

    // solve route module
    std::unique_ptr<TspSolutionRoute> solve_route(std::unique_ptr<TspRoutePlanParam> route_plan_param);
    // void solve_multi_route(std::vector<std::unique_ptr<TspRoutePlanParam>> route_plan_params);
    std::unique_ptr<TspSolutionRoute> solve(int matrix_type);
    std::unique_ptr<TspSolutionRoute> solve();
};

#endif // TSP_MODEL_H
