/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#include "algo_alns_repair_rank.h"

#include <gtest/gtest.h>

#include <algorithm>
#include <cstddef>
#include <random>
#include <vector>

namespace {
/**
 * @brief 暴力解: 按全序取最优的 min(k, n) 个, 按全序升序输出(front 最优)
 */
std::vector<RepairCellView> brute_force_top_k(std::vector<RepairCellView> candidates, const int k) {
  if (k <= 0) {
    return {};
  }
  std::sort(candidates.begin(), candidates.end(), RepairCellViewLess{});
  candidates.resize(std::min(static_cast<std::size_t>(k), candidates.size()));
  return candidates;
}

/**
 * @brief 断言两个视图序列逐字段相等
 */
void expect_views_eq(const std::vector<RepairCellView>& actual,
                     const std::vector<RepairCellView>& expected) {
  ASSERT_EQ(actual.size(), expected.size());
  for (std::size_t i = 0; i < actual.size(); ++i) {
    EXPECT_EQ(actual[i].delta, expected[i].delta) << "index=" << i;
    EXPECT_EQ(actual[i].segment_ind, expected[i].segment_ind) << "index=" << i;
    EXPECT_EQ(actual[i].load_ind, expected[i].load_ind) << "index=" << i;
  }
}

/**
 * @brief 生成确定性的候选集合: 每对 (segment, load) 唯一, delta 可重复
 */
std::vector<RepairCellView> make_candidates(const int segment_count, const int load_count,
                                            const unsigned int seed) {
  std::mt19937 engine(seed);
  std::uniform_int_distribution<int> delta_dist(0, 10);
  std::vector<RepairCellView> candidates;
  candidates.reserve(static_cast<std::size_t>(segment_count) * load_count);
  for (int s = 0; s < segment_count; ++s) {
    for (int l = 0; l < load_count; ++l) {
      candidates.push_back(RepairCellView{static_cast<double>(delta_dist(engine)), s, l});
    }
  }
  return candidates;
}
}  // namespace

TEST(AlgoAlnsRepairRankTest, EmptyCandidates) {
  EXPECT_TRUE(RepairRank::select_top_k({}, 1).empty());
  EXPECT_TRUE(RepairRank::select_top_k({}, 0).empty());
}

TEST(AlgoAlnsRepairRankTest, NonPositiveK) {
  const auto candidates = make_candidates(3, 2, 7);
  EXPECT_TRUE(RepairRank::select_top_k(candidates, 0).empty());
  EXPECT_TRUE(RepairRank::select_top_k(candidates, -1).empty());
}

TEST(AlgoAlnsRepairRankTest, MatchesBruteForceForAllK) {
  const auto candidates = make_candidates(5, 4, 20261126);
  ASSERT_EQ(candidates.size(), 20U);
  for (const int k : {-3, 0, 1, 2, 3, 7, 19, 20, 21, 100}) {
    expect_views_eq(RepairRank::select_top_k(candidates, k), brute_force_top_k(candidates, k));
  }
}

TEST(AlgoAlnsRepairRankTest, AscendingOrderAndBestAtFront) {
  const auto candidates = make_candidates(4, 3, 37);
  const auto top_k = RepairRank::select_top_k(candidates, 5);
  ASSERT_EQ(top_k.size(), 5U);
  for (std::size_t i = 1; i < top_k.size(); ++i) {
    EXPECT_FALSE(RepairCellViewLess{}(top_k[i], top_k[i - 1])) << "index=" << i;
  }
  // front() 是保留集合中全序最小的一个
  const auto all_sorted = brute_force_top_k(candidates, static_cast<int>(candidates.size()));
  EXPECT_EQ(top_k.front().delta, all_sorted.front().delta);
}

TEST(AlgoAlnsRepairRankTest, TieBreakBySegmentThenLoad) {
  // 升序: 同一 delta 下 segment_ind 小者在前, 再按 load_ind 小者在前
  std::vector<RepairCellView> candidates{
      {1.0, 0, 0}, {1.0, 0, 1}, {1.0, 1, 0}, {1.0, 1, 2}, {2.0, 0, 1}, {0.5, 9, 9},
  };
  const auto top_k = RepairRank::select_top_k(candidates, static_cast<int>(candidates.size()));
  const std::vector<RepairCellView> expected{
      {0.5, 9, 9}, {1.0, 0, 0}, {1.0, 0, 1}, {1.0, 1, 0}, {1.0, 1, 2}, {2.0, 0, 1},
  };
  expect_views_eq(top_k, expected);
}

TEST(AlgoAlnsRepairRankTest, LessIsStrictTotalOrder) {
  const RepairCellView a{1.0, 0, 0};
  const RepairCellView b{1.0, 0, 1};
  const RepairCellView c{1.0, 1, 0};
  const RepairCellView d{2.0, 0, 0};
  // 非自反
  EXPECT_FALSE(RepairCellViewLess{}(a, a));
  // 反对称
  EXPECT_TRUE(RepairCellViewLess{}(a, b));
  EXPECT_FALSE(RepairCellViewLess{}(b, a));
  EXPECT_TRUE(RepairCellViewLess{}(b, c));
  EXPECT_TRUE(RepairCellViewLess{}(a, c));
  // delta 优先
  EXPECT_TRUE(RepairCellViewLess{}(c, d));
  EXPECT_FALSE(RepairCellViewLess{}(d, c));
}

TEST(AlgoAlnsRepairRankTest, KEqualsCandidateCount) {
  const auto candidates = make_candidates(3, 3, 11);
  const auto top_k = RepairRank::select_top_k(candidates, 9);
  expect_views_eq(top_k, brute_force_top_k(candidates, 9));
}
