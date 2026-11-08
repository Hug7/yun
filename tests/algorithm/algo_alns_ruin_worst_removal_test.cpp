/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>

#include <algorithm>
#include <cstddef>
#include <vector>

#include "algo_alns_ruin_worst_removal_order.h"

namespace {

/**
 * @brief 采样 `count` 次, 统计首个被选中下标出现的次数
 */
std::vector<int> sample_first_selected(const std::vector<double>& gains, const int sample_count,
                                       const double noise_exponent, const unsigned int seed) {
  RandomUtils random(seed);
  std::vector<int> counter(gains.size(), 0);
  for (int u = 0; u < sample_count; ++u) {
    const std::vector<std::size_t> selected =
        select_worst_removal_indices(gains, 1, noise_exponent, random);
    ++counter[selected.front()];
  }
  return counter;
}

}  // namespace

TEST(SelectWorstRemovalIndicesTest, HugeNoiseExponentDegradesToTopK) {
  RandomUtils random(37);
  const std::vector<double> gains{1.0, 5.0, 3.0, 9.0, 2.0};

  const std::vector<std::size_t> selected = select_worst_removal_indices(gains, 2, 1.0e6, random);

  ASSERT_EQ(selected.size(), 2U);
  EXPECT_EQ(selected[0], 3U);
  EXPECT_EQ(selected[1], 1U);
}

TEST(SelectWorstRemovalIndicesTest, SameSeedProducesSameResult) {
  const std::vector<double> gains{1.0, 5.0, 3.0, 9.0, 2.0};
  RandomUtils random_a(37);
  RandomUtils random_b(37);

  EXPECT_EQ(select_worst_removal_indices(gains, 3, 3.0, random_a),
            select_worst_removal_indices(gains, 3, 3.0, random_b));
}

TEST(SelectWorstRemovalIndicesTest, CountLargerThanCandidatesReturnsAllIndicesOnce) {
  RandomUtils random(37);
  const std::vector<double> gains{1.0, 5.0, 3.0};

  std::vector<std::size_t> selected = select_worst_removal_indices(gains, 5, 3.0, random);
  std::ranges::sort(selected);

  EXPECT_EQ(selected, (std::vector<std::size_t>{0U, 1U, 2U}));
}

TEST(SelectWorstRemovalIndicesTest, ZeroCountReturnsEmpty) {
  RandomUtils random(37);
  const std::vector<double> gains{1.0, 5.0, 3.0};

  EXPECT_TRUE(select_worst_removal_indices(gains, 0, 3.0, random).empty());
}

TEST(SelectWorstRemovalIndicesTest, DefaultNoiseExponentBiasesToLargestGain) {
  const std::vector<double> gains{1.0, 5.0, 3.0, 9.0, 2.0};
  constexpr int kSampleCount = 1000;

  const std::vector<int> counter = sample_first_selected(gains, kSampleCount, 3.0, 37);
  constexpr std::size_t kTopInd = 3;

  // p=3, n=5 时首位命中概率的理论值是 0.2^(1/3) ≈ 0.585
  EXPECT_GT(counter[kTopInd], kSampleCount * 45 / 100);
  for (std::size_t u = 0; u < counter.size(); ++u) {
    if (u == kTopInd) {
      continue;
    }
    EXPECT_LT(counter[u], counter[kTopInd]);
  }
}

TEST(SelectWorstRemovalIndicesTest, EqualGainsReturnDistinctIndices) {
  RandomUtils random(37);
  const std::vector<double> gains{5.0, 5.0, 5.0, 5.0};

  std::vector<std::size_t> selected = select_worst_removal_indices(gains, 3, 3.0, random);
  std::ranges::sort(selected);

  ASSERT_EQ(selected.size(), 3U);
  for (const std::size_t ind : selected) {
    EXPECT_LT(ind, gains.size());
  }
  EXPECT_EQ(std::ranges::adjacent_find(selected), selected.end());
}
