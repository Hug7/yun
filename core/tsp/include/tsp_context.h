#ifndef TSP_CONTEXT_H
#define TSP_CONTEXT_H

#include <iostream>
#include <map>
#include <unordered_map>
#include <vector>
#include <tuple>
#include <string>
#include <spdlog/spdlog.h>

#include "random_utils.h"
#include "tsp_location.h"
#include "cost_matrix.h"
#include "tsp_param.h"

class TspContext
{
private:
    TspContext(std::vector<TspLocation *> &locations, CostMatrix *cost_matrix, TspParam *tsp_param);

public:
    std::vector<TspLocation *> locations;
    int visit_loc_count;
    TspLocation *start_loc;
    TspLocation *last_loc;
    CostMatrix *cost_matrix;
    TspParam *tsp_param;
    // 工具类
    std::unique_ptr<RandomUtils> random_utils;
    // 优先级相关成员变量
    std::unordered_map<int, std::tuple<int, int>> priority_2_ind_range_map;  // 访问优先级对应可选择的位置
    std::vector<int> ind_2_priority_map;                                     // route索引对应的优先级
    std::vector<int> prioritiy_list;  // 优先级集合
    // 分组相关成员变量
    std::unordered_map<int, std::unordered_map<std::string, int>> priority_2_group_mark_2_loc_count_map;                   // 不同优先级、分组标识对应的站点数量

    // 随机获取优先级
    int random_priority();

    // 初始化数据的后处理
    void post_init();

    // 处理优先级约束相关的数据
    void post_init_process_priority();

    // 处理分组约束相关的数据
    void post_init_process_group();

    // 推导约束是否开启
    void post_init_infer_constraint();

    static TspContext *create_tsp_context(std::vector<TspLocation *> &locations, CostMatrix *cost_matrix, TspParam *tsp_param, TspLocation *start_loc, TspLocation *last_loc);

    ~TspContext();
};

#endif // TSP_CONTEXT_H
