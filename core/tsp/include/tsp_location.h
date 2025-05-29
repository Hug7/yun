#ifndef TSP_LOCATION_H
#define TSP_LOCATION_H

#include <string>
#include <vector>
#include <memory>

#include "time_window.h"
#include "default_parameter.h"

class TspLocation
{
public:
    const int matrix_ind; // 全局索引用于定位距离矩阵
    const std::string id; // 唯一名称
    int loc_ind;          // tsp location集合的位置索引
    std::vector<TimeWindow *> time_windows;
    int work_time;                               // 固定耗时
    int priority = DEFAULT_PRIORITY;             // 访问优先级: 按照优先级从低到高访问，default=INT_MAX，有效取值范围[0, INT_MAX - 1]，声明为负数时代表提货，否则为卸货
    std::string group_mark = DEFAULT_GROUP_MARK; // 分组标识: 相同分组标识的站点访问顺序要相邻，default="xxx"不限制站点访问顺序
    int group_priority = DEFAULT_GROUP_PRIORITY; // 分组分优先级: 相同运输标识的站点访问顺序要相邻，default=INT_MAX，有效取值范围[0, INT_MAX - 1]
    bool is_default_group_mark = true;           // 是否是默认分组标识
    bool is_tmp_loc;                             // 是否是临时站点
    std::vector<std::string> remarks;            // 备注

    TspLocation(std::string &id, int matrix_ind) : id(id), matrix_ind(matrix_ind), loc_ind(-1) {}

    TspLocation(std::string &id, int matrix_ind, int loc_ind) : id(id), matrix_ind(matrix_ind), loc_ind(loc_ind) {}

    ~TspLocation();
    
    // 更新关联变量
    void update_relation_vars();

    // 重置分组相关变量
    void reset_group_info();

    static TspLocation *create_tmp_location(std::string &id, int matrix_ind, int loc_ind);
};

#endif // TSP_LOCATION_H
