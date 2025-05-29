#ifndef TSP_CANDIDATE_SOLUTION_H
#define TSP_CANDIDATE_SOLUTION_H

#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
#include <memory>

class CandidateSol
{
public:
    // 邻域动作
    std::vector<int> neighborhood_action;
    // 表层邻域动作
    std::vector<std::vector<int>> neighborhood_surface_action;
    // 邻域方法名称
    std::string neighborhood_operator;
    long dist;
    // 目标值
    double obj_val;
    // 惩罚值
    double punishment_val;
    // 总的目标值
    double total_val;

    CandidateSol() {};
    std::unique_ptr<CandidateSol> copy();

    bool operator<(std::unique_ptr<CandidateSol> &candidate_sol) const
    {
        return this->total_val < candidate_sol->total_val;
    }
};

#endif // TSP_CANDIDATE_SOLUTION_H
