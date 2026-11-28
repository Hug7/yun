/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#include "algo_alns_repair_rank.h"

#include <algorithm>
#include <cstddef>

namespace RepairRank {

std::vector<RepairCellView> select_top_k(std::vector<RepairCellView> candidates, const int k) {
  if (k <= 0 || candidates.empty()) {
    return {};
  }

  const auto size = static_cast<int>(candidates.size());
  const int count = std::min(k, size);
  const auto middle = candidates.begin() + static_cast<std::ptrdiff_t>(count);
  // 按全序取最优的 count 个并保持升序: front 最优、back() 最差
  std::ranges::partial_sort(candidates, middle, RepairCellViewLess{});
  candidates.resize(static_cast<std::size_t>(count));
  return candidates;
}

}  // namespace RepairRank
