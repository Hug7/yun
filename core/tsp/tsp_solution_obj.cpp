#include "tsp_solution_obj.h"

/**
 * tsp solution obj
 */

class TspSolutionObjRest : public TspSolutionObjInterface
{
    double compute(TspContext *tsp_context, std::unique_ptr<TspSolutionRouteAttr> &tsp_route_attr) override
    {
        tsp_route_attr->obj_val = 0;
        return 0;
    }
};

class TspSolutionObjDistance : public TspSolutionObjInterface
{
private:
    double factor = 1.;

public:
    TspSolutionObjDistance(double factor) : factor(factor) {};

    double compute(TspContext *tsp_context, std::unique_ptr<TspSolutionRouteAttr> &tsp_route_attr) override
    {
        double cur_obj_val = this->factor * tsp_route_attr->total_cost_dist;
        tsp_route_attr->obj_val += cur_obj_val;
        return cur_obj_val;
    }
};

class TspSolutionObjTime : public TspSolutionObjInterface
{
private:
    double factor = 1.;

public:
    TspSolutionObjTime(double factor) : factor(factor) {};

    double compute(TspContext *tsp_context, std::unique_ptr<TspSolutionRouteAttr> &tsp_route_attr) override
    {
        double cur_obj_val = this->factor * tsp_route_attr->total_cost_time;
        tsp_route_attr->obj_val += cur_obj_val;
        return cur_obj_val;
    }
};

class TspSolutionObjTimeWindow : public TspSolutionObjInterface
{
private:
    double factor = 1.;

public:
    TspSolutionObjTimeWindow(double factor) : factor(factor) {};

    double compute(TspContext *tsp_context, std::unique_ptr<TspSolutionRouteAttr> &tsp_route_attr) override
    {
        double cur_obj_val = 0.0;
        for (int u = 0; u < tsp_route_attr->wait_times.size(); ++u)
        {
            if (tsp_route_attr->over_times[u] > 0)
            {
                cur_obj_val += OVER_PENALTY + F_MAX / (1 + exp(-K * (tsp_route_attr->over_times[u] - BAR_X)));
            }
            else if (tsp_route_attr->wait_times[u] > 0)
            {
                cur_obj_val += OVER_PENALTY + F_MAX / (1 + exp(-K * (tsp_route_attr->wait_times[u] - BAR_X)));
            }
        }
        cur_obj_val = this->factor * cur_obj_val;
        tsp_route_attr->obj_val += cur_obj_val;
        return cur_obj_val;
    }
};

std::unique_ptr<TspSolutionObjUtil> create_tsp_obj_util(TspContext *&tsp_context)
{
    std::vector<std::unique_ptr<TspSolutionObjInterface>> tsp_sol_obj_coll;
    tsp_sol_obj_coll.push_back(std::make_unique<TspSolutionObjRest>());
    // enable total dist as opt object
    if (tsp_context->tsp_param->object_enable_total_dist)
    {
        tsp_sol_obj_coll.push_back(std::make_unique<TspSolutionObjDistance>(tsp_context->tsp_param->object_factor_total_dist));
    }
    // enable total time as opt object
    if (tsp_context->tsp_param->object_enable_total_time)
    {
        tsp_sol_obj_coll.push_back(std::make_unique<TspSolutionObjTime>(tsp_context->tsp_param->object_factor_total_time));
    }
    // enable time window penalty as opt object
    tsp_sol_obj_coll.push_back(std::make_unique<TspSolutionObjTimeWindow>(tsp_context->tsp_param->penalty_factor_wait_time));

    return std::make_unique<TspSolutionObjUtil>(std::move(tsp_sol_obj_coll));
}
