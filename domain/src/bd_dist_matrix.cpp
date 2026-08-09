/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#include "bd_dist_matrix.h"

// ====== implement of DistMatrix ======
DistMatrix::DistMatrix(
    const int dist_matrix_ind, const int matrix_len) : dist_matrix_ind(dist_matrix_ind),
                                                       matrix_len(matrix_len)
{
    this->arr = new DistTime[matrix_len * matrix_len];
    this->arr[0] = DistTime(0, 0);
    for (int u = 1; u < matrix_len; ++u)
    {
        this->arr[u] = DistTime(0, 0);
        this->arr[u * matrix_len] = DistTime(0, 0);
    }

    for (int u = 1; u < matrix_len; ++u)
    {
        this->arr[u * matrix_len + u] = DistTime(0, 0);
    }
}

DistMatrix::~DistMatrix()
{
    delete[] this->arr;
}

long DistMatrix::get_dist(const int from_ind, const int to_ind) const
{
    return this->arr[from_ind * this->matrix_len + to_ind].dist;
}

long DistMatrix::get_time(const int from_ind, const int to_ind) const
{
    return this->arr[from_ind * this->matrix_len + to_ind].time;
}

DistTime DistMatrix::get_dist_time(const int from_ind, const int to_ind) const
{
    return this->arr[from_ind * this->matrix_len + to_ind];
}

void DistMatrix::set_dist_time(const int from_ind, const int to_ind, const long dist, const long time)
{
    this->arr[from_ind * this->matrix_len + to_ind] = DistTime(dist, time);
}

// ====== implement of DistMatrixManager ======
DistMatrixManager::~DistMatrixManager()
{
    for (auto &cost_matrix_code : this->dist_matrix_code_map)
    {
        delete cost_matrix_code.second;
    }
    this->dist_matrix_code_map.clear();
    for (auto &cost_matrix : this->dist_matrixes)
    {
        delete cost_matrix;
    }
    this->dist_matrixes.clear();
}

void DistMatrixManager::create_dist_matrix_code(const std::string &dist_matrix_code, const std::string &dist_matrix_code_name)
{
    if (this->dist_matrix_code_map.count(dist_matrix_code))
    {
        throw std::runtime_error("CostMatrixManager: cost matrix code " + dist_matrix_code + " already exists.");
    }
    DistMatrixCode *dist_matrix_code_ptr = new DistMatrixCode(dist_matrix_code, dist_matrix_code_name, this->generate_index->next());
    this->dist_matrix_code_map[dist_matrix_code] = dist_matrix_code_ptr;
    this->dist_matrixes.push_back(new DistMatrix(dist_matrix_code_ptr->ind, this->matrix_len));
}

DistMatrixCode *DistMatrixManager::get_dist_matrix_code(const std::string &dist_matrix_code)
{
    auto dist_matrix_code_it = this->dist_matrix_code_map.find(dist_matrix_code);
    if (dist_matrix_code_it == this->dist_matrix_code_map.end())
    {
        throw std::runtime_error("CostMatrixManager: cost matrix code " + dist_matrix_code + " does not exist.");
    }
    return dist_matrix_code_it->second;
}

void DistMatrixManager::set_dist_time(const std::string &dist_matrix_code, const int row_ind, const int col_ind, const long cost_dist, const long cost_time)
{
    auto dist_matrix_code_it = this->dist_matrix_code_map.find(dist_matrix_code);

    if (dist_matrix_code_it == this->dist_matrix_code_map.end())
    {
        throw std::runtime_error("CostMatrixManager: cost matrix code " + dist_matrix_code + " does not exist.");
    }
    int dist_matrix_code_ind = dist_matrix_code_it->second->ind;

    this->dist_matrixes[dist_matrix_code_ind]->set_dist_time(row_ind, col_ind, cost_dist, cost_time);
}

DistMatrix *DistMatrixManager::get_dist_matrix(DistMatrixCode *dist_matrix_code)
{
    return this->dist_matrixes[dist_matrix_code->ind];
}
