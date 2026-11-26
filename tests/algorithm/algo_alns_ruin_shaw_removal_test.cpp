/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdlib>
#include <limits>
#include <utility>
#include <vector>

#include "algo_alns_ruin_shaw_removal_order.h"

namespace {

/**
 * @brief 用对称距离矩阵构造 order_distance
 * @details 矩阵按局部下标索引, 需覆盖 `[0, order_count)` 的任意两两组合; 按值捕获以持有矩阵生命周期
 */
ShawOrderDistanceFunc make_distance_func(std::vector<std::vector<double>> spatial_dists,
                                         std::vector<std::vector<double>> time_dists) {
  return [spatial_dists = std::move(spatial_dists), time_dists = std::move(time_dists)](
             const int lhs, const int rhs) {
    return std::pair<double, double>{
        spatial_dists[static_cast<std::size_t>(lhs)][static_cast<std::size_t>(rhs)],
        time_dists[static_cast<std::size_t>(lhs)][static_cast<std::size_t>(rhs)]};
  };
}

/**
 * @brief 构造 N×N 矩阵
 */
std::vector<std::vector<double>> make_matrix(const int n, const double value) {
  return std::vector<std::vector<double>>(static_cast<std::size_t>(n),
                                          std::vector<double>(static_cast<std::size_t>(n), value));
}

/**
 * @brief 复现被测函数的种子抽取: 先消耗一次 random_int
 */
int probe_seed(const int order_count, const unsigned int seed) {
  RandomUtils random(seed);
  return random.random_int(0, order_count - 1);
}

/**
 * @brief 按「到已选集合的最小距离」贪心选下一个, 同距离取最小下标
 * @details 贪心指数极大时选择退化为该确定性贪心
 */
std::vector<int> greedy_nearest_sequence(const int order_count,
                                         const std::vector<std::vector<double>>& dists,
                                         const int seed_order, const int count) {
  std::vector<int> sequence{seed_order};
  std::vector<bool> removed(static_cast<std::size_t>(order_count), false);
  removed[static_cast<std::size_t>(seed_order)] = true;
  while (static_cast<int>(sequence.size()) < count &&
         static_cast<int>(sequence.size()) < order_count) {
    int best_order = -1;
    double best_dist = std::numeric_limits<double>::infinity();
    for (int order = 0; order < order_count; ++order) {
      if (removed[static_cast<std::size_t>(order)]) {
        continue;
      }
      double cur_dist = std::numeric_limits<double>::infinity();
      for (const int selected_order : sequence) {
        cur_dist = std::min(
            cur_dist,
            dists[static_cast<std::size_t>(order)][static_cast<std::size_t>(selected_order)]);
      }
      if (cur_dist < best_dist) {
        best_dist = cur_dist;
        best_order = order;
      }
    }
    sequence.push_back(best_order);
    removed[static_cast<std::size_t>(best_order)] = true;
  }
  return sequence;
}

}  // namespace

// ===== min_max_normalize =====
TEST(MinMaxNormalizeTest, MapsMinAndMaxToUnitInterval) {
  const std::vector<double> values{0.0, 5.0, 10.0};

  const std::vector<double> result = min_max_normalize(values);
  ASSERT_EQ(result.size(), 3U);
  EXPECT_DOUBLE_EQ(result[0], 0.0);
  EXPECT_DOUBLE_EQ(result[1], 0.5);
  EXPECT_DOUBLE_EQ(result[2], 1.0);
}

TEST(MinMaxNormalizeTest, InfiniteValuesMapToOne) {
  const std::vector<double> values{1.0, std::numeric_limits<double>::infinity(), 3.0};

  const std::vector<double> result = min_max_normalize(values);
  ASSERT_EQ(result.size(), 3U);
  EXPECT_DOUBLE_EQ(result[0], 0.0);
  EXPECT_DOUBLE_EQ(result[1], 1.0);
  EXPECT_DOUBLE_EQ(result[2], 1.0);
}

TEST(MinMaxNormalizeTest, AllEqualFiniteValuesReturnZero) {
  const std::vector<double> values{2.0, 2.0, 2.0};

  const std::vector<double> result = min_max_normalize(values);
  ASSERT_EQ(result.size(), 3U);
  EXPECT_DOUBLE_EQ(result[0], 0.0);
  EXPECT_DOUBLE_EQ(result[1], 0.0);
  EXPECT_DOUBLE_EQ(result[2], 0.0);
}

TEST(MinMaxNormalizeTest, AllInfiniteValuesReturnOne) {
  const std::vector<double> values{std::numeric_limits<double>::infinity(),
                                   std::numeric_limits<double>::infinity()};

  const std::vector<double> result = min_max_normalize(values);
  ASSERT_EQ(result.size(), 2U);
  EXPECT_DOUBLE_EQ(result[0], 1.0);
  EXPECT_DOUBLE_EQ(result[1], 1.0);
}

TEST(MinMaxNormalizeTest, EmptyInputReturnsEmpty) { EXPECT_TRUE(min_max_normalize({}).empty()); }

// ===== select_shaw_removal_order_indices =====
TEST(SelectShawRemovalOrderIndicesTest, ZeroCountReturnsEmpty) {
  RandomUtils random(37);
  const ShawOrderDistanceFunc order_distance =
      make_distance_func(make_matrix(3, 1.0), make_matrix(3, 0.0));

  EXPECT_TRUE(
      select_shaw_removal_order_indices(3, order_distance, 0, 0.5, 0.5, 3.0, random).empty());
}

TEST(SelectShawRemovalOrderIndicesTest, ZeroOrderCountReturnsEmpty) {
  RandomUtils random(37);
  const ShawOrderDistanceFunc order_distance =
      make_distance_func(make_matrix(0, 1.0), make_matrix(0, 0.0));

  EXPECT_TRUE(
      select_shaw_removal_order_indices(0, order_distance, 3, 0.5, 0.5, 3.0, random).empty());
}

TEST(SelectShawRemovalOrderIndicesTest, SingleCountReturnsSeed) {
  constexpr int kOrderCount = 5;
  RandomUtils random(37);
  const ShawOrderDistanceFunc order_distance =
      make_distance_func(make_matrix(kOrderCount, 1.0), make_matrix(kOrderCount, 0.0));

  const std::vector<int> selected =
      select_shaw_removal_order_indices(kOrderCount, order_distance, 1, 0.5, 0.5, 3.0, random);

  ASSERT_EQ(selected.size(), 1U);
  EXPECT_EQ(selected.front(), probe_seed(kOrderCount, 37));
}

TEST(SelectShawRemovalOrderIndicesTest, CountLargerThanOrdersReturnsAllOnce) {
  constexpr int kOrderCount = 4;
  RandomUtils random(37);
  const ShawOrderDistanceFunc order_distance =
      make_distance_func(make_matrix(kOrderCount, 1.0), make_matrix(kOrderCount, 0.0));

  std::vector<int> selected =
      select_shaw_removal_order_indices(kOrderCount, order_distance, 10, 0.5, 0.5, 3.0, random);
  std::ranges::sort(selected);

  EXPECT_EQ(selected, (std::vector<int>{0, 1, 2, 3}));
}

TEST(SelectShawRemovalOrderIndicesTest, SameSeedProducesSameResult) {
  constexpr int kOrderCount = 5;
  std::vector<std::vector<double>> spatial_dists = make_matrix(kOrderCount, 0.0);
  std::vector<std::vector<double>> time_dists = make_matrix(kOrderCount, 0.0);
  for (int lhs = 0; lhs < kOrderCount; ++lhs) {
    for (int rhs = 0; rhs < kOrderCount; ++rhs) {
      spatial_dists[static_cast<std::size_t>(lhs)][static_cast<std::size_t>(rhs)] =
          std::abs(lhs - rhs);
      time_dists[static_cast<std::size_t>(lhs)][static_cast<std::size_t>(rhs)] =
          3.0 * std::abs(lhs - rhs);
    }
  }
  const ShawOrderDistanceFunc order_distance = make_distance_func(spatial_dists, time_dists);

  RandomUtils random_a(37);
  RandomUtils random_b(37);
  EXPECT_EQ(
      select_shaw_removal_order_indices(kOrderCount, order_distance, 3, 0.5, 0.5, 3.0, random_a),
      select_shaw_removal_order_indices(kOrderCount, order_distance, 3, 0.5, 0.5, 3.0, random_b));
}

TEST(SelectShawRemovalOrderIndicesTest, HugeNoiseExponentFollowsNearestNeighbours) {
  constexpr int kOrderCount = 6;
  constexpr int kSelectCount = 4;
  // 空间距离取直线下标差, 时间距离全部相等: 归一化后退化为按下标距离的最近邻链
  std::vector<std::vector<double>> spatial_dists = make_matrix(kOrderCount, 0.0);
  std::vector<std::vector<double>> time_dists = make_matrix(kOrderCount, 0.0);
  for (int lhs = 0; lhs < kOrderCount; ++lhs) {
    for (int rhs = 0; rhs < kOrderCount; ++rhs) {
      spatial_dists[static_cast<std::size_t>(lhs)][static_cast<std::size_t>(rhs)] =
          std::abs(lhs - rhs);
    }
  }
  const ShawOrderDistanceFunc order_distance = make_distance_func(spatial_dists, time_dists);
  RandomUtils random(37);

  const std::vector<int> selected = select_shaw_removal_order_indices(
      kOrderCount, order_distance, kSelectCount, 0.5, 0.5, 1.0e6, random);
  const std::vector<int> expected = greedy_nearest_sequence(
      kOrderCount, spatial_dists, probe_seed(kOrderCount, 37), kSelectCount);

  EXPECT_EQ(selected, expected);
}

TEST(SelectShawRemovalOrderIndicesTest, TimeWeightOnlyUsesTimeDistance) {
  constexpr int kOrderCount = 6;
  constexpr int kSelectCount = 3;
  // 空间距离与时间距离的最近邻不一致: 只开时间项时应沿时间距离聚类(订单 2 与所有人时间最近)
  std::vector<std::vector<double>> spatial_dists = make_matrix(kOrderCount, 0.0);
  std::vector<std::vector<double>> time_dists = make_matrix(kOrderCount, 100.0);
  for (int lhs = 0; lhs < kOrderCount; ++lhs) {
    for (int rhs = 0; rhs < kOrderCount; ++rhs) {
      spatial_dists[static_cast<std::size_t>(lhs)][static_cast<std::size_t>(rhs)] =
          std::abs(lhs - rhs);
      // 订单 2 的提卸时间与所有订单重叠; 其余订单之间时间窗完全分离
      if (lhs == 2 || rhs == 2 || lhs == rhs) {
        time_dists[static_cast<std::size_t>(lhs)][static_cast<std::size_t>(rhs)] = 0.0;
      }
    }
  }
  const ShawOrderDistanceFunc order_distance = make_distance_func(spatial_dists, time_dists);
  RandomUtils random(37);

  const std::vector<int> selected = select_shaw_removal_order_indices(
      kOrderCount, order_distance, kSelectCount, 0.0, 1.0, 1.0e6, random);
  const std::vector<int> expected =
      greedy_nearest_sequence(kOrderCount, time_dists, probe_seed(kOrderCount, 37), kSelectCount);

  EXPECT_EQ(selected, expected);
  EXPECT_EQ(selected.size(), static_cast<std::size_t>(kSelectCount));
}
