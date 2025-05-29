#include <iostream>
#include <vector>
#include <string>
#include <climits>
#include <chrono>
#include <memory>

#include "rapidcsv.h"

#include "log.h"
#include "location.h"
#include "cost_matrix.h"
#include "tsp_param.h"
#include "tsp_context.h"
#include "tsp_solution_obj.h"
#include "tsp_model.h"

// std::vector<TspLocation *> load_location(std::string &file_path)
// {
//     rapidcsv::Document location_doc(file_path + "/" + "Location.csv");

//     int row_count = location_doc.GetRowCount();
//     spdlog::info("Location number of row is {}", row_count);

//     std::vector<TspLocation *> locations;
//     for (int u = 0; u < row_count; ++u)
//     {
//         std::string location_id = location_doc.GetCell<std::string>("LocationId", u);
//         TspLocation *cur_loc = new TspLocation(u);
//         cur_loc->set_unique_name(location_id);
//         locations.push_back(cur_loc);
//     }
//     return locations;
// }

// CostMatrix *load_dist_matrix(std::string &file_path, std::vector<TspLocation *> &locations)
// {
//     std::sort(locations.begin(), locations.end(), [](TspLocation *a, TspLocation *b)
//               { return a->ind < b->ind; });

//     std::unordered_map<std::string, std::unordered_map<std::string, std::pair<long, long>>> dist_time_matrix_map;

//     rapidcsv::Document doc(file_path + "/" + "DistanceTimeMatrix.csv");

//     int row_count = doc.GetRowCount();
//     spdlog::info("DistanceTimeMatrix number of row is {}", row_count);

//     for (int u = 0; u < row_count; ++u)
//     {
//         std::string from_node = doc.GetCell<std::string>("from_node", u);
//         std::string to_node = doc.GetCell<std::string>("to_node", u);
//         long distance = doc.GetCell<long>("0_distance", u);
//         long duration = doc.GetCell<long>("0_duration", u);

//         dist_time_matrix_map[from_node][to_node] = std::make_pair(distance, duration);
//     }

//     // 初始化距离矩阵和时间矩阵
//     int location_count = locations.size();
//     std::vector<std::vector<long>> dist_arr(location_count, std::vector<long>(location_count, LONG_MAX));
//     std::vector<std::vector<long>> time_arr(location_count, std::vector<long>(location_count, LONG_MAX));

//     for (size_t u = 0; u < location_count; u++)
//     {
//         for (size_t m = 0; m < location_count; m++)
//         {
//             if (u != m)
//             {
//                 dist_arr[u][m] = dist_time_matrix_map[locations[u]->unique_name][locations[m]->unique_name].first;
//                 time_arr[u][m] = dist_time_matrix_map[locations[u]->unique_name][locations[m]->unique_name].second;
//             }
//         }
//     }
//     CostMatrix *cost_matrix = new CostMatrix(dist_arr, time_arr);
//     spdlog::info("Load distance & time matrix successfully");
//     return cost_matrix;
// }

std::unique_ptr<Calendar> create_work_calendar()
{
    std::unique_ptr<PeriodCalendar> work_calendar = std::make_unique<PeriodCalendar>();
    work_calendar->mondays.push_back(std::make_pair(0, 2 * 3600));
    work_calendar->mondays.push_back(std::make_pair(3 * 3600, 6 * 3600));
    work_calendar->tuesdays.push_back(std::make_pair(0, 2 * 3600));
    work_calendar->tuesdays.push_back(std::make_pair(3 * 3600, 6 * 3600));
    work_calendar->wednesdays.push_back(std::make_pair(0, 2 * 3600));
    work_calendar->wednesdays.push_back(std::make_pair(3 * 3600, 6 * 3600));
    work_calendar->thursdays.push_back(std::make_pair(0, 2 * 3600));
    work_calendar->thursdays.push_back(std::make_pair(3 * 3600, 6 * 3600));
    work_calendar->fridays.push_back(std::make_pair(0, 2 * 3600));
    work_calendar->fridays.push_back(std::make_pair(3 * 3600, 6 * 3600));
    work_calendar->saturdays.push_back(std::make_pair(0, 2 * 3600));
    work_calendar->saturdays.push_back(std::make_pair(3 * 3600, 6 * 3600));
    work_calendar->sundays.push_back(std::make_pair(0, 2 * 3600));
    work_calendar->sundays.push_back(std::make_pair(3 * 3600, 6 * 3600));

    // work_calendar->mondays.push_back(std::make_pair(0, 6 * 3600));
    // work_calendar->tuesdays.push_back(std::make_pair(0, 6 * 3600));
    // work_calendar->wednesdays.push_back(std::make_pair(0, 6 * 3600));
    // work_calendar->thursdays.push_back(std::make_pair(0, 6 * 3600));
    // work_calendar->fridays.push_back(std::make_pair(0, 6 * 3600));
    // work_calendar->saturdays.push_back(std::make_pair(0, 6 * 3600));
    // work_calendar->sundays.push_back(std::make_pair(0, 6 * 3600));

    return std::make_unique<Calendar>(work_calendar);
}

std::vector<Location *> create_locations()
{
    std::vector<std::string> location_ids = {
        "A1", "B1", "B2", "B3"
        };
    std::vector<Location *> locations;

    for (std::string &location_id : location_ids)
    {
        Location *cur_loc = new Location(location_id);
        cur_loc->pick_calendar = create_work_calendar();
        cur_loc->delivery_calendar = create_work_calendar();
        cur_loc->work_time = 600;

        locations.push_back(cur_loc);
    }

    return locations;
}

std::vector<Order *> create_orders()
{
    std::vector<Order *> orders;
    Order *order1 = new Order("1", "A1", "B1", 0, 0, 
        std::make_unique<TimeWindowDateTime>(1745573688, 1745660088), 
        std::make_unique<TimeWindowDateTime>(1745573688, 1745660088));
    order1->group_mark = "K";
    order1->group_priority = 3;
    orders.push_back(order1);
    Order *order2 = new Order("2", "A1", "B2", 0, 0, 
        std::make_unique<TimeWindowDateTime>(1745573688, 1745660088), 
        std::make_unique<TimeWindowDateTime>(1745573688, 1745660088));
    order2->group_mark = "K";
    order2->group_priority = 2;
    orders.push_back(order2);
    orders.push_back(new Order("3", "A1", "B3", 0, 0, 
        std::make_unique<TimeWindowDateTime>(1745573688, 1745660088), 
        std::make_unique<TimeWindowDateTime>(1745573688, 1745660088)));

    return orders;
}

int main()
{
    /**
     * conan install . --build=missing -s build_type=Debug --output-folder=build
     * conan create . --build=missing -s build_type=Debug
     * 
     * conan install . --build=missing -s build_type=Release --output-folder=build
     * conan create . --build=missing -s build_type=Release
     */

    LoggerConfig logger_config;
    logger_config.log_level = "debug";
    logger_config.log_file = "tsp_main.log";
    logger_config.prefix = "tsp";
    
    init_logger(logger_config);
    std::string file_path = "E:\\code\\yun\\yun\\resources\\case1";
    // std::vector<TspLocation *> locations = load_location(file_path);
    // CostMatrix *cost_matrix = load_dist_matrix(file_path, locations);
    // TspParam *tsp_param = new TspParam(false, false, 1.);
    // TspContext *tsp_context = TspContext::create_tsp_context(locations, cost_matrix, tsp_param);
    // std::unique_ptr<TspSolutionObjUtil> tsp_sol_obj_util = create_default_tsp_obj_util();
    // spdlog::info(tsp_context->locations.size());

    // auto start = std::chrono::high_resolution_clock::now();
    // fast_solve(tsp_context, std::move(tsp_sol_obj_util));
    // auto end = std::chrono::high_resolution_clock::now();
    // auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    // spdlog::info("算法总耗时 = {} ms", duration.count());

    // tsp_context->release();

    // auto start = std::chrono::high_resolution_clock::now();
    
    TspModel *tsp_model = new TspModel();
    std::vector<Location *> locations = create_locations();
    tsp_model->set_locations(locations);
    std::vector<Order *> orders = create_orders();
    tsp_model->set_orders(orders);
    
    tsp_model->load_file(file_path);
    auto start = std::chrono::high_resolution_clock::now();
    std::unique_ptr<TspRoutePlanParam> route_plan_param = std::make_unique<TspRoutePlanParam>();
    // route_plan_param->order_ids = {"D20240108301&VXIT&HFHF001", "DN202504030197&VXIT&HFHF001"};
    tsp_model->solve_route(std::move(route_plan_param));
    delete tsp_model;
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    spdlog::info("算法总耗时 = {} ms", duration.count());

    return 0;
}
