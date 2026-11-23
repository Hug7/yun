/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#include "bdm_dist_matrix.h"

#include <limits>
#include <stdexcept>

namespace {

std::size_t checked_entry_count(const int matrix_len) {
  if (matrix_len <= 0) {
    throw std::invalid_argument("DistMatrix: matrix_len must be positive");
  }

  const auto n = static_cast<std::size_t>(matrix_len);
  if (n > std::numeric_limits<std::size_t>::max() / n) {
    throw std::length_error("DistMatrix: matrix_len is too large");
  }
  return n * n;
}

}  // namespace

// ====== implement of DistMatrix ======
DistMatrix::DistMatrix(const int dist_matrix_ind, const int matrix_len)
    : arr_(checked_entry_count(matrix_len)),
      dist_matrix_ind(dist_matrix_ind),
      matrix_len(matrix_len) {
  const auto n = static_cast<std::size_t>(matrix_len);
  this->arr_[0] = DistTime(0, 0);
  for (std::size_t u = 1; u < n; ++u) {
    this->arr_[u] = DistTime(0, 0);
    this->arr_[u * n] = DistTime(0, 0);
    this->arr_[u * n + u] = DistTime(0, 0);
  }
}

DistMatrix::~DistMatrix() = default;

std::size_t DistMatrix::offset(const int from_ind, const int to_ind) const noexcept {
  return static_cast<std::size_t>(from_ind) * static_cast<std::size_t>(this->matrix_len) +
         static_cast<std::size_t>(to_ind);
}

std::int64_t DistMatrix::get_dist(const int from_ind, const int to_ind) const {
  return this->arr_[this->offset(from_ind, to_ind)].dist;
}

std::int64_t DistMatrix::get_time(const int from_ind, const int to_ind) const {
  return this->arr_[this->offset(from_ind, to_ind)].time;
}

DistTime DistMatrix::get_dist_time(const int from_ind, const int to_ind) const {
  return this->arr_[this->offset(from_ind, to_ind)];
}

void DistMatrix::set_dist_time(const int from_ind, const int to_ind, const std::int64_t dist,
                               const std::int64_t time) {
  if (from_ind < 0 || from_ind >= this->matrix_len || to_ind < 0 || to_ind >= this->matrix_len) {
    throw std::out_of_range("DistMatrix::set_dist_time: index out of range");
  }
  this->arr_[this->offset(from_ind, to_ind)] = DistTime(dist, time);
}

// ====== implement of DistMatrixManager ======
DistMatrixManager::~DistMatrixManager() {
  for (auto& cost_matrix_code : this->dist_matrix_code_map) {
    delete cost_matrix_code.second;
  }
  this->dist_matrix_code_map.clear();
  for (auto& cost_matrix : this->dist_matrices) {
    delete cost_matrix;
  }
  this->dist_matrices.clear();
}

void DistMatrixManager::create_dist_matrix_code(const std::string& dist_matrix_code,
                                                const std::string& dist_matrix_code_name) {
  if (this->dist_matrix_code_map.count(dist_matrix_code)) {
    throw std::runtime_error("CostMatrixManager: cost matrix code " + dist_matrix_code +
                             " already exists.");
  }

  const int code_ind = this->generate_index->next();
  auto code = std::make_unique<DistMatrixCode>(dist_matrix_code, dist_matrix_code_name, code_ind);
  auto matrix = std::make_unique<DistMatrix>(code_ind, this->matrix_len);

  const auto slot = static_cast<std::size_t>(code_ind);
  this->dist_matrices.resize(slot + 1, nullptr);
  this->dist_matrices[slot] = matrix.get();

  try {
    const bool inserted = this->dist_matrix_code_map.emplace(dist_matrix_code, code.get()).second;
    if (!inserted) {
      this->dist_matrices[slot] = nullptr;
      throw std::runtime_error("CostMatrixManager: cost matrix code " + dist_matrix_code +
                               " already exists.");
    }
  } catch (...) {
    this->dist_matrices[slot] = nullptr;
    throw;
  }

  code.release();
  matrix.release();
}

DistMatrixCode* DistMatrixManager::get_dist_matrix_code(const std::string& dist_matrix_code) {
  auto dist_matrix_code_it = this->dist_matrix_code_map.find(dist_matrix_code);
  if (dist_matrix_code_it == this->dist_matrix_code_map.end()) {
    throw std::runtime_error("CostMatrixManager: cost matrix code " + dist_matrix_code +
                             " does not exist.");
  }
  return dist_matrix_code_it->second;
}

void DistMatrixManager::set_dist_time(const std::string& dist_matrix_code, const int row_ind,
                                      const int col_ind, const std::int64_t cost_dist,
                                      const std::int64_t cost_time) {
  auto dist_matrix_code_it = this->dist_matrix_code_map.find(dist_matrix_code);

  if (dist_matrix_code_it == this->dist_matrix_code_map.end()) {
    throw std::runtime_error("CostMatrixManager: cost matrix code " + dist_matrix_code +
                             " does not exist.");
  }
  DistMatrix* dist_matrix = this->get_dist_matrix(dist_matrix_code_it->second);
  dist_matrix->set_dist_time(row_ind, col_ind, cost_dist, cost_time);
}

DistMatrix* DistMatrixManager::get_dist_matrix(DistMatrixCode* dist_matrix_code) {
  if (dist_matrix_code == nullptr) {
    throw std::invalid_argument("DistMatrixManager: dist matrix code is null");
  }
  if (dist_matrix_code->ind < 0 ||
      static_cast<std::size_t>(dist_matrix_code->ind) >= this->dist_matrices.size()) {
    throw std::out_of_range("DistMatrixManager: invalid dist matrix code");
  }

  DistMatrix* dist_matrix = this->dist_matrices[dist_matrix_code->ind];
  if (dist_matrix == nullptr) {
    throw std::out_of_range("DistMatrixManager: dist matrix has not been created");
  }
  return dist_matrix;
}
