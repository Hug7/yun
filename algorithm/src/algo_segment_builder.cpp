/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "algo_segment_builder.h"

#include <ranges>

std::vector<Segment*> SegmentBuilder::call(const Solution* sol) {
  std::vector<Segment*> segments;
  for (const auto& order : sol->unassigned_orders | std::views::values) {
    segments.emplace_back(new Segment({order}));
  }

  return segments;
}
