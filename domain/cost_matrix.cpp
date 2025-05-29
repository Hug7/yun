#include "cost_matrix.h"


CostMatrix::CostMatrix(std::vector<std::vector<long>> &dist_arr, std::vector<std::vector<long>> &time_arr)
{
    int ori_row = dist_arr.size();
    this->rows = ori_row + 1;
    this->cols = ori_row + 1;
    this->dist_arr = new long *[this->rows];
    this->time_arr = new long *[this->rows];
    for (int u = 0; u < this->rows; ++u)
    {
        this->dist_arr[u] = new long[this->cols];
        this->time_arr[u] = new long[this->cols];
        if (u < ori_row)
        {
            for (int v = 0; v < ori_row; ++v)
            {
                this->dist_arr[u][v] = dist_arr[u][v];
                this->time_arr[u][v] = time_arr[u][v];
            }

            for (int v = ori_row; v < this->rows; ++v)
            {
                this->dist_arr[u][v] = 0;
                this->time_arr[u][v] = 0;
            }
        }
        else
        {
            for (int v = 0; v < this->rows; ++v)
            {
                this->dist_arr[u][v] = 0;
                this->time_arr[u][v] = 0;
            }
        }
    }
}

CostMatrix::~CostMatrix()
{
    for (int u = 0; u < this->rows; ++u)
    {
        delete[] this->dist_arr[u];
        delete[] this->time_arr[u];
    }
    delete[] this->dist_arr;
    delete[] this->time_arr;
}

