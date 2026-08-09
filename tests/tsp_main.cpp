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
    std::string file_path = "../resources/case1";
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
