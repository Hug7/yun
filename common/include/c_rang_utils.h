/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <unordered_set>
#include <vector>

namespace RangeUtils {

inline std::vector<int> range_vector(const int len) {
  std::vector<int> res;
  res.reserve(len);
  for (int u = 0; u < len; u++) {
    res.push_back(u);
  }
  return res;
}

inline std::unordered_set<int> range_set(const int len) {
  std::unordered_set<int> res;
  res.reserve(len);
  for (int u = 0; u < len; u++) {
    res.insert(u);
  }
  return res;
}

}  // namespace RangeUtils
