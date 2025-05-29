#include "tsp_context.h"

int TspContext::random_priority()
{
    if (this->prioritiy_list.size() == 1)
    {
        return this->prioritiy_list[0];
    }
    int ind = random_utils->random_int(0, this->prioritiy_list.size() - 1);
    return this->prioritiy_list[ind];
}

TspContext::TspContext(std::vector<TspLocation *> &locations, CostMatrix *cost_matrix, TspParam *tsp_param)
{
    this->locations = std::vector<TspLocation *>(locations.begin(), locations.end());
    this->cost_matrix = cost_matrix;
    this->tsp_param = tsp_param;

    this->random_utils = std::make_unique<RandomUtils>(100);
}

TspContext *TspContext::create_tsp_context(std::vector<TspLocation *> &locations, CostMatrix *cost_matrix, TspParam *tsp_param, TspLocation *start_loc, TspLocation *last_loc)
{
    TspContext *tsp_context = new TspContext(locations, cost_matrix, tsp_param);
    tsp_context->start_loc = start_loc;
    tsp_context->last_loc = last_loc;

    tsp_context->post_init();
    return tsp_context;
}

TspContext::~TspContext()
{
    for (auto &loc : this->locations)
    {
        delete loc;
    }
    this->locations.clear();

    this->start_loc = nullptr;
    this->last_loc = nullptr;

    this->cost_matrix = nullptr;
    this->tsp_param = nullptr;

    this->priority_2_ind_range_map.clear();
    this->ind_2_priority_map.clear();
    this->prioritiy_list.clear();
}
