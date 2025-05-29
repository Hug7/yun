#include <unordered_set>
#include <ranges>

#include "tsp_model.h"

TspLocation *create_pickup_tsp_location(Location *location,
                                        Order *order,
                                        std::unordered_map<std::string, int> loc_id_to_ind_map,
                                        std::vector<TimeWindow *> &time_windows)
{
    TspLocation *tsp_location = new TspLocation(location->id, loc_id_to_ind_map[location->id]);
    tsp_location->time_windows = time_windows;
    tsp_location->work_time = location->work_time + order->pickup_work_time;
    tsp_location->priority = -1;
    tsp_location->is_tmp_loc = false;
    tsp_location->remarks.push_back(order->id);

    return tsp_location;
}

TspLocation *create_delivery_tsp_location(Location *location,
                                          Order *order,
                                          std::unordered_map<std::string, int> loc_id_to_ind_map,
                                          std::vector<TimeWindow *> &time_windows)
{
    TspLocation *tsp_location = new TspLocation(location->id, loc_id_to_ind_map[location->id]);
    tsp_location->time_windows = time_windows;
    tsp_location->work_time = location->work_time + order->delivery_work_time;
    tsp_location->priority = order->priority;
    tsp_location->group_mark = order->group_mark;
    tsp_location->group_priority = order->group_priority;
    tsp_location->is_tmp_loc = false;
    tsp_location->remarks.push_back(order->id);

    return tsp_location;
}

std::vector<TimeWindow *> intersection_time_windows(std::vector<TimeWindow *> time_windows1, std::unique_ptr<TimeWindowDateTime> &time_window_datetime)
{
    std::vector<TimeWindow *> intersection_time_windows_arr;
    TimeWindow *tw2 = time_window_datetime->convert();
    for (auto &tw1 : time_windows1)
    {
        TimeWindow *cur_inters_tw = tw1->intersection(tw2);
        if (cur_inters_tw != nullptr)
        {
            intersection_time_windows_arr.push_back(cur_inters_tw);
        }
    }
    delete tw2;
    return intersection_time_windows_arr;
}

// std::vector<Location *> load_location(std::string &file_path)
// {
//     rapidcsv::Document location_doc(file_path + "/" + "Location.csv");

//     int row_count = location_doc.GetRowCount();
//     spdlog::info("Location number of row is {}", row_count);

//     std::vector<Location *> locations;
//     for (int u = 0; u < row_count; ++u)
//     {
//         std::string location_id = location_doc.GetCell<std::string>("LocationId", u);
//         Location *cur_loc = new Location(location_id, u);
//         locations.push_back(cur_loc);
//     }
//     return locations;
// }

CostMatrix *load_dist_matrix(int matrix_type_prefix, std::string &file_path, std::vector<Location *> &locations, std::unordered_map<std::string, int> &loc_id_to_ind_map)
{
    std::unordered_map<std::string, std::unordered_map<std::string, std::pair<long, long>>> dist_time_matrix_map;

    rapidcsv::Document doc(file_path + "/" + "DistanceTimeMatrix.csv");

    int row_count = doc.GetRowCount();
    std::string distance_col = "distance_" + std::to_string(matrix_type_prefix);
    std::string duration_col = "duration_" + std::to_string(matrix_type_prefix);
    // 如果列不存在
    if (doc.GetColumnIdx(distance_col) == -1 || doc.GetColumnIdx(duration_col) == -1)
    {
        return nullptr;
    }
    spdlog::info("DistanceTimeMatrix number of row is {}", row_count);
    // 初始化距离矩阵和时间矩阵
    int location_count = locations.size();
    std::vector<std::vector<long>> dist_arr(location_count, std::vector<long>(location_count, LONG_MAX));
    std::vector<std::vector<long>> time_arr(location_count, std::vector<long>(location_count, LONG_MAX));
    for (int u = 0; u < row_count; ++u)
    {
        std::string from_node = doc.GetCell<std::string>("from_node", u);
        if (!loc_id_to_ind_map.count(from_node))
        {
            continue;
        }
        std::string to_node = doc.GetCell<std::string>("to_node", u);
        if (!loc_id_to_ind_map.count(to_node))
        {
            continue;
        }
        int from_node_ind = loc_id_to_ind_map[from_node];
        int to_node_ind = loc_id_to_ind_map[to_node];
        dist_arr[from_node_ind][to_node_ind] = doc.GetCell<long>(distance_col, u);
        time_arr[from_node_ind][to_node_ind] = doc.GetCell<long>(duration_col, u);
    }
    CostMatrix *cost_matrix = new CostMatrix(dist_arr, time_arr);
    spdlog::info("Load distance & time matrix successfully, matrix type = {}", matrix_type_prefix);
    return std::move(cost_matrix);
}

TspModel::TspModel()
{
    this->tsp_param = new TspParam();
    this->loc_count = 0;
    this->order_count = 0;
    this->virtual_loc = new Location("virtual_loc");
}

void TspModel::set_locations(std::vector<Location *> &locations)
{
    this->locations = std::vector<Location *>(locations.begin(), locations.end());
    this->loc_count = 0;
    this->loc_id_to_ind_map.clear();
    this->loc_ind_to_id_map.clear();
    for (const Location *location : this->locations)
    {
        this->loc_id_to_ind_map[location->id] = this->loc_count;
        this->loc_ind_to_id_map[this->loc_count] = location->id;
        this->loc_count++;
    }
}

void TspModel::set_orders(std::vector<Order *> &orders)
{
    this->orders = std::vector<Order *>(orders.begin(), orders.end());
    this->order_count = 0;
    this->order_id_to_ind_map.clear();
    for (const Order *order : this->orders)
    {
        this->order_id_to_ind_map[order->id] = this->order_count;
        this->order_count++;
    }
}

void TspModel::load_file(std::string &file_path)
{
    for (int matrix_type = 0; matrix_type < 4; matrix_type++)
    {
        CostMatrix *cost_matrix = load_dist_matrix(matrix_type, file_path, this->locations, this->loc_id_to_ind_map);
        if (cost_matrix == nullptr)
        {
            continue;
        }
        this->cost_matrixs[matrix_type] = cost_matrix;
    }
}

Location *&TspModel::get_location(const std::string &location_id)
{
    auto loc_iter = this->loc_id_to_ind_map.find(location_id);
    if (loc_iter == this->loc_id_to_ind_map.end())
    {
        spdlog::error("location_id = {} not found", location_id);
        throw std::runtime_error("location_id not found");
    }
    int matrix_ind = loc_iter->second;
    return this->locations[matrix_ind];
}

Order *&TspModel::get_order(const std::string &order_id)
{
    auto order_iter = this->order_id_to_ind_map.find(order_id);
    if (order_iter == this->order_id_to_ind_map.end())
    {
        spdlog::error("order_id = {} not found", order_id);
        throw std::runtime_error("order_id not found");
    }
    int order_ind = order_iter->second;
    return this->orders[order_ind];
}

std::unique_ptr<TspSolutionRoute> TspModel::solve_route(std::unique_ptr<TspRoutePlanParam> route_plan_param)
{
    // if order ids is empty, then solve all orders
    std::vector<std::string> order_ids = std::vector<std::string>(route_plan_param->order_ids.begin(), route_plan_param->order_ids.end());
    if (route_plan_param->order_ids.empty())
    {
        for (auto &order : this->orders)
        {
            order_ids.push_back(order->id);
        }
    }

    std::unordered_map<std::string, TspLocation *> pick_locations;
    std::unordered_map<OrderHash, TspLocation *> delivery_locations;
    // merge pickup and delivery locations
    for (const auto &order_id : order_ids)
    {
        Order *&order = this->get_order(order_id);
        // 将订单的提货时间窗 和 提货站点的时间窗进行交集
        Location *&pickup_loc = this->get_location(order->pickup_loc_id);
        if (pick_locations.find(order->pickup_loc_id) != pick_locations.end())
        {
            pick_locations[pickup_loc->id]->work_time += order->pickup_work_time;
            std::vector<TimeWindow *> tmp_time_windows = intersection_time_windows(pick_locations[pickup_loc->id]->time_windows, order->pickup_time_window);
            for (auto &tw : pick_locations[pickup_loc->id]->time_windows)
            {
                delete tw;
            }
            pick_locations[pickup_loc->id]->time_windows.clear();
            pick_locations[pickup_loc->id]->time_windows = tmp_time_windows;
            pick_locations[pickup_loc->id]->remarks.push_back(order->id);
            if (pick_locations[pickup_loc->id]->time_windows.size() == 0)
            {
                spdlog::error("pickup_loc_id = {} time_windows is empty", order->pickup_loc_id);
                throw std::runtime_error("pickup_loc_id time_windows is empty");
            }
        }
        else
        {
            std::vector<TimeWindow *> pickup_time_windows = pickup_loc->pick_calendar->intersection(order->pickup_time_window);
            TspLocation *tsp_pick_loc = create_pickup_tsp_location(pickup_loc, order, this->loc_id_to_ind_map, pickup_time_windows);
            pick_locations[pickup_loc->id] = tsp_pick_loc;
        }

        // 将订单的卸货时间窗 和 提货站点的时间
        Location *&delivery_loc = this->get_location(order->delivery_loc_id);
        OrderHash cur_order_hash = order->get_order_hash();
        if (delivery_locations.find(cur_order_hash) != delivery_locations.end())
        {
            delivery_locations[cur_order_hash]->work_time += order->delivery_work_time;
            std::vector<TimeWindow *> tmp_time_windows = intersection_time_windows(delivery_locations[cur_order_hash]->time_windows, order->delivery_time_window);
            for (auto &tw : delivery_locations[cur_order_hash]->time_windows)
            {
                delete tw;
            }
            delivery_locations[cur_order_hash]->time_windows.clear();
            delivery_locations[cur_order_hash]->time_windows = tmp_time_windows;
            delivery_locations[cur_order_hash]->remarks.push_back(order->id);
            if (delivery_locations[cur_order_hash]->time_windows.size() == 0)
            {
                spdlog::error("delivery_loc_id = {} time_windows is empty", order->delivery_loc_id);
                throw std::runtime_error("delivery_loc_id time_windows is empty");
            }
        }
        else
        {
            std::vector<TimeWindow *> delivery_time_windows = delivery_loc->delivery_calendar->intersection(order->delivery_time_window);
            TspLocation *tsp_delivery_loc = create_delivery_tsp_location(delivery_loc, order, this->loc_id_to_ind_map, delivery_time_windows);
            delivery_locations[cur_order_hash] = tsp_delivery_loc;
        }
    }

    spdlog::info("waiting solve pickup location count = {}, delivery location count = {}", pick_locations.size(), delivery_locations.size());
    spdlog::info("waiting solve matrix type = {}", route_plan_param->matrix_type);
    int matrix_type = route_plan_param->matrix_type;
    if (matrix_type == -1)
    {
        matrix_type = this->cost_matrixs.begin()->first;
    }
    // 合并提货和卸货站点
    std::vector<TspLocation *> tsp_locations;
    for (const auto &loc_pair : pick_locations)
    {
        tsp_locations.push_back(loc_pair.second);
    }
    for (const auto &loc_pair : delivery_locations)
    {
        tsp_locations.push_back(loc_pair.second);
    }
    pick_locations.clear();
    delivery_locations.clear();
    // 设置始发和结尾的虚拟站点
    TspLocation *virtual_tsp_loc = TspLocation::create_tmp_location(this->virtual_loc->id, this->loc_count, tsp_locations.size());
    tsp_locations.push_back(virtual_tsp_loc);

    TspContext *tsp_context = TspContext::create_tsp_context(tsp_locations, this->cost_matrixs[matrix_type], this->tsp_param, virtual_tsp_loc, virtual_tsp_loc);
    // 目标函数
    std::unique_ptr<TspSolutionObjUtil> tsp_sol_obj_util = create_tsp_obj_util(tsp_context);
    // 约束
    std::unique_ptr<TspConstraintHardUtils> tsp_constraint_hard_utils = create_tsp_constraint_hard_utils(tsp_context);

    std::unique_ptr<TspSolutionRoute> sol = fast_solve(tsp_context, std::move(tsp_sol_obj_util), std::move(tsp_constraint_hard_utils));
    spdlog::info("Best solution obj val = {:.2f}", sol->attr->obj_val);

    // 释放 tsp_context
    delete tsp_context;

    return std::move(sol);
}

std::unique_ptr<TspSolutionRoute> TspModel::solve(int matrix_type)
{
    std::unique_ptr<TspRoutePlanParam> route_plan_param = std::make_unique<TspRoutePlanParam>();
    route_plan_param->order_ids = std::vector<std::string>();
    route_plan_param->matrix_type = matrix_type;
    return this->solve_route(std::move(route_plan_param));
}

std::unique_ptr<TspSolutionRoute> TspModel::solve()
{
    return this->solve(-1);
}

TspModel::~TspModel()
{
    for (auto loc : this->locations)
    {
        delete loc;
    }
    this->locations.clear();

    for (auto order : this->orders)
    {
        delete order;
    }
    this->locations.clear();

    this->loc_id_to_ind_map.clear();
    this->loc_ind_to_id_map.clear();

    for (auto cost_matrix : this->cost_matrixs)
    {
        delete cost_matrix.second;
    }
    this->cost_matrixs.clear();

    delete this->tsp_param;
}
