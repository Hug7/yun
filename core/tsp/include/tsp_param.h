#ifndef TSP_PARAM_H
#define TSP_PARAM_H

struct TspParam
{
    /**
     * object
     */
    bool object_enable_total_dist = true; // 距离目标是否启用, default = true
    double object_factor_total_dist = 1.; // 距离目标系数, default = 1

    bool object_enable_total_time = false; // 耗时目标是否启用, default = false
    double object_factor_total_time = 1.; // 耗时目标系数, default = 1

    /**
     * constraint
     */
    bool constraint_enable_time_window = true; // 是否使用时间窗约束, default = true
    bool constraint_enable_group = true;      // 是否使用分组约束, default = false

    /**
     * penalty
     */
    double penalty_factor_wait_time = 1.; // 等待时间惩罚

    TspParam() {}

    ~TspParam() {}
};

#endif // TSP_PARAM_H
