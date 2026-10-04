/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <algorithm>
#include <memory>
#include <random>
#include <vector>

#include "c_rang_utils.h"

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

  inline double random_double() {
    std::uniform_real_distribution<double> dist(0, 1);
    return dist(gen);
  }

  inline double random_double(const double down, const double upper) {
    std::uniform_real_distribution<double> dist(down, upper);
    return dist(gen);
  }

  inline int random_int(const int factor, const double down, const double upper) {
    std::uniform_real_distribution<double> dist(down, upper);
    return static_cast<int>(factor * dist(gen));
  }

  template <typename T>
  inline void shuffle(std::vector<T>& vec) {
    std::shuffle(vec.begin(), vec.end(), gen);
  }

  inline std::vector<int> shuffled_range(const int len) {
    std::vector<int> vec = RangeUtils::range_vector(len);
    std::ranges::shuffle(vec, gen);
    return vec;
  }
};
