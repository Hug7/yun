#pragma once
#ifndef TSP_SOLUTION_OBJ_H
#define TSP_SOLUTION_OBJ_H

#include <vector>
#include <memory>

#include "tsp_context.h"
#include "tsp_solution_route_attr.h"

#define WAIT_PENALTY 5.0E5          // 等待时间惩罚
#define OVER_PENALTY 5.0E5 + 2000.0 // 超时时间惩罚
#define F_MAX 2000.0                // 最大的惩罚函数值
#define K 0.005                     // 函数的斜率，越大越陡峭
#define BAR_X 900.0                 // 中值，即指数和对数切换的临界点

class TspSolutionObjInterface
{
public:
    TspSolutionObjInterface() {};
    virtual ~TspSolutionObjInterface() {}
    virtual double compute(TspContext *tsp_context, std::unique_ptr<TspSolutionRouteAttr> &tsp_route_attr) = 0;
};

class TspSolutionObjUtil
{
private:
    std::vector<std::unique_ptr<TspSolutionObjInterface>> tsp_sol_objs;

public:
    TspSolutionObjUtil(std::vector<std::unique_ptr<TspSolutionObjInterface>> tsp_sol_objs)
    {
        this->tsp_sol_objs = std::move(tsp_sol_objs);
    }

    ~TspSolutionObjUtil()
    {
        this->tsp_sol_objs.clear();
    }

    double compute(TspContext *tsp_context, std::unique_ptr<TspSolutionRouteAttr> &tsp_route_attr)
    {
        double obj_val = 0.0;
        for (auto &tsp_sol_obj : this->tsp_sol_objs)
        {
            obj_val += tsp_sol_obj->compute(tsp_context, tsp_route_attr);
        }
        return obj_val;
    }
};

std::unique_ptr<TspSolutionObjUtil> create_tsp_obj_util(TspContext *&tsp_context);

#endif // TSP_SOLUTION_OBJ_H
