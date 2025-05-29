#ifndef TSP_SOLUTION_NODE_H
#define TSP_SOLUTION_NODE_H

#include <vector>
#include <memory>

#include "tsp_location.h"
#include "time_window_plan.h"

class TspSolutionNode
{
public:
    int matrix_ind;
    int loc_ind;
    long cost_dist;
    long cost_time;
    int work_time;
    std::vector<std::unique_ptr<TimeWindowPlan>> time_windows;

    TspSolutionNode();
    TspSolutionNode(TspLocation *&tsp_loc);
    std::unique_ptr<TspSolutionNode> deep_copy();
    std::vector<std::unique_ptr<TimeWindowPlan>> deep_copy_time_windows();
    ~TspSolutionNode();
    bool meet_time_window();
    bool zero_wait_time();
    bool zero_over_time();
};

#endif // TSP_SOLUTION_NODE_H
