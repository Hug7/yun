/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <algorithm>
#include <random>
#include <vector>
#include <memory>

class RandomUtils {
 public:
  using UPtr = std::unique_ptr<RandomUtils>;

  std::mt19937 gen;

  explicit RandomUtils(const unsigned int seed) : gen(seed) {}

  ~RandomUtils() = default;

  inline int random_int(const int down, const int upper) {
    std::uniform_int_distribution<int> dist(down, upper);
    return dist(gen);
  }

  inline double random_double(const double min, const double max) {
    std::uniform_real_distribution<double> dist(min, max);
    return dist(gen);
  }

  template <typename T>
  inline void shuffle(std::vector<T>& vec) {
    std::shuffle(vec.begin(), vec.end(), gen);
  }
};
