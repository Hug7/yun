#ifndef COST_MATRIX_H
#define COST_MATRIX_H

#include <vector>
#include <memory>

#include "location.h"

class CostMatrix
{
public:
    int rows;
    int cols;
    long **dist_arr;
    long **time_arr;

    CostMatrix(std::vector<std::vector<long>> &dist_arr, std::vector<std::vector<long>> &time_arr);

    ~CostMatrix();

    long get_dist(int from_ind, int to_ind);

    long get_time(int from_ind, int to_ind);
};

inline long CostMatrix::get_dist(int from_ind, int to_ind)
{
    return this->dist_arr[from_ind][to_ind];
};

inline long CostMatrix::get_time(int from_ind, int to_ind)
{
    return this->time_arr[from_ind][to_ind];
};

#endif // COST_MATRIX_H
