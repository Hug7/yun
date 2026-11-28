/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#include "algo_segment_builder.h"

#include <ranges>
#include <vector>

std::vector<Segment> SegmentBuilder::call(const Solution* sol) {
  std::vector<Segment> segments;
  for (const auto& order : sol->unassigned_orders | std::views::values) {
    segments.emplace_back(std::vector<const Order*>{order});
  }

  return segments;
}
