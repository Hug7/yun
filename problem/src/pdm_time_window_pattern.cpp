/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#include "pdm_time_window_pattern.h"

namespace time_window_pattern {
namespace {
constexpr std::size_t K_HASH_SEED = 0x9e3779b97f4a7c15ULL;
}  // namespace

std::size_t Hash::operator()(const std::vector<TimeWindow>& tws) const noexcept {
  std::size_t seed = tws.size();
  for (const auto& tw : tws) {
    seed ^= std::hash<std::int64_t>{}(tw.early) + K_HASH_SEED + (seed << 6) + (seed >> 2);
    seed ^= std::hash<std::int64_t>{}(tw.late) + K_HASH_SEED + (seed << 6) + (seed >> 2);
  }
  return seed;
}

bool Equal::operator()(const std::vector<TimeWindow>& lhs,
                       const std::vector<TimeWindow>& rhs) const noexcept {
  if (lhs.size() != rhs.size()) {
    return false;
  }
  for (std::size_t u = 0; u < lhs.size(); u++) {
    if (lhs[u].early != rhs[u].early || lhs[u].late != rhs[u].late) {
      return false;
    }
  }
  return true;
}
}  // namespace time_window_pattern

int TimeWindowPatternRegistry::intern(const std::vector<TimeWindow>& tws) {
  const auto [it, inserted] = this->ids_.try_emplace(tws, static_cast<int>(this->ids_.size()));
  return it->second;
}
