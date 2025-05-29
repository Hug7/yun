#include "tsp_context.h"

void TspContext::post_init_process_priority()
{
    // 统计各个优先级的站点数量
    std::map<int, int> priority_count_map;
    for (const auto &loc : this->locations)
    {
        if (loc->is_tmp_loc)
        {
            continue;
        }
        priority_count_map[loc->priority] += 1;
    }
    // 推导优先级所在范围 & 索引应该选择的优先级
    int begin_ind = 1;
    this->ind_2_priority_map.push_back(-1); // 始发站的优先级为-1
    for (const auto &map_pair : priority_count_map)
    {
        for (int u = 0; u < map_pair.second; ++u)
        {
            this->ind_2_priority_map.push_back(map_pair.first);
        }
        this->priority_2_ind_range_map[map_pair.first] = std::make_tuple(begin_ind, begin_ind + map_pair.second - 1);
        this->prioritiy_list.push_back(map_pair.first);
        begin_ind += map_pair.second;
    }
    this->ind_2_priority_map.push_back(-1); // 终点站的优先级为-1
}

void TspContext::post_init_process_group()
{
    // 统计各个优先级+分组标识的站点数量
    for (const auto &loc : this->locations)
    {
        if (loc->is_tmp_loc)
        {
            continue;
        }
        this->priority_2_group_mark_2_loc_count_map[loc->priority][loc->group_mark] += 1;
    }
    // 如果站点优先级+分组标识的站点数量为1，则将该站点分组标识置为默认值
    for (const auto &loc : this->locations)
    {
        if (this->priority_2_group_mark_2_loc_count_map[loc->priority][loc->group_mark] <= 1)
        {
            this->priority_2_group_mark_2_loc_count_map[loc->priority][loc->group_mark] = 0;
            loc->reset_group_info();
            this->priority_2_group_mark_2_loc_count_map[loc->priority][loc->group_mark] += 1;
        }
    }
}

void TspContext::post_init_infer_constraint()
{
    // 推导分组约束是否关闭
    if (this->tsp_param->constraint_enable_group)
    {
        bool enable_flag = false;
        for (const auto &group_mark_2_loc_count_map : this->priority_2_group_mark_2_loc_count_map)
        {
            for (auto group_mark_2_loc_count : group_mark_2_loc_count_map.second)
            {
                if (group_mark_2_loc_count.first == DEFAULT_GROUP_MARK)
                {
                    continue;
                }
                if (group_mark_2_loc_count.second > 1)
                {
                    enable_flag = true;
                    break;
                }
            }
            if (enable_flag)
            {
                break;
            }
        }
        if (!enable_flag)
        {
            spdlog::debug("close group of constraint!");
            this->tsp_param->constraint_enable_group = false;
        }
    }
}

void TspContext::post_init()
{
    // 更新tsp location的索引 和 关联变量
    for (int u = 0; u < this->locations.size(); ++u)
    {
        this->locations[u]->loc_ind = u;
        this->locations[u]->update_relation_vars();
    }
    // 更新站点数量
    this->visit_loc_count = this->locations.size() - 1 + 2;
    // 处理优先级约束相关的数据
    this->post_init_process_priority();
    // 处理分组约束相关的数据
    this->post_init_process_group();
    // 推导约束是否开启
    this->post_init_infer_constraint();
}
