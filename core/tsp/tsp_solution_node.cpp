#include "tsp_solution_node.h"

/**
 * tsp solution node
 */
TspSolutionNode::TspSolutionNode()
{
    this->matrix_ind = -1;
    this->loc_ind = -1;
    this->cost_dist = 0;
    this->cost_time = 0;
    this->work_time = 0;
    this->time_windows = std::vector<std::unique_ptr<TimeWindowPlan>>();
}

TspSolutionNode::TspSolutionNode(TspLocation *&loc)
{
    this->matrix_ind = loc->matrix_ind;
    this->loc_ind = loc->loc_ind;
    this->cost_dist = 0;
    this->cost_time = 0;
    this->work_time = loc->work_time;
    this->time_windows = std::vector<std::unique_ptr<TimeWindowPlan>>();
}

std::unique_ptr<TspSolutionNode> TspSolutionNode::deep_copy()
{
    std::unique_ptr<TspSolutionNode> tsp_sol_node = std::make_unique<TspSolutionNode>();
    tsp_sol_node->matrix_ind = this->matrix_ind;
    tsp_sol_node->loc_ind = this->loc_ind;
    tsp_sol_node->cost_dist = this->cost_dist;
    tsp_sol_node->cost_time = this->cost_time;
    tsp_sol_node->work_time = this->work_time;
    for (std::unique_ptr<TimeWindowPlan> &cur_time_window : this->time_windows)
    {
        tsp_sol_node->time_windows.push_back(cur_time_window->deep_copy());
    }
    return std::move(tsp_sol_node);
}

std::vector<std::unique_ptr<TimeWindowPlan>> TspSolutionNode::deep_copy_time_windows()
{
    std::vector<std::unique_ptr<TimeWindowPlan>> time_windows_copy;
    for (std::unique_ptr<TimeWindowPlan> &cur_time_window : this->time_windows)
    {
        time_windows_copy.push_back(cur_time_window->deep_copy());
    }
    return std::move(time_windows_copy);
}

TspSolutionNode::~TspSolutionNode()
{
    this->time_windows.clear();
}

bool TspSolutionNode::meet_time_window()
{
    return this->time_windows[0]->wait_time <= 0 && this->time_windows[0]->over_time <= 0;
}

bool TspSolutionNode::zero_wait_time()
{
    return this->time_windows[0]->wait_time <= 0;
}

bool TspSolutionNode::zero_over_time()
{
    return this->time_windows[0]->over_time <= 0;
}
