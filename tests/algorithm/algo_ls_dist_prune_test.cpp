/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#include "algo_ls_dist_prune.h"

#include <gtest/gtest.h>

#include <cstdint>
#include <optional>
#include <vector>

#include "algo_ls_move.h"
#include "bdm_dist_matrix.h"

namespace {

constexpr int kLocationCount = 6;

/**
 * @brief 造一张对称距离矩阵：|i - j| * 10
 */
DistMatrix make_matrix() {
  DistMatrix matrix(0, kLocationCount);
  for (int from = 0; from < kLocationCount; ++from) {
    for (int to = 0; to < kLocationCount; ++to) {
      matrix.set_dist_time(from, to, std::abs(from - to) * 10, std::abs(from - to) * 10);
    }
  }
  return matrix;
}

/**
 * @brief 造一个车次快照：START(0) -> PICK(0) -> units... -> END(5)
 */
LsDistPrune::RouteSnapshot make_route(const DistMatrix* matrix, std::vector<int> units,
                                      const bool feasible = true) {
  return LsDistPrune::RouteSnapshot{.dist_matrix = matrix,
                                    .pick_loc_ind = 0,
                                    .end_loc_ind = 5,
                                    .unit_loc_inds = std::move(units),
                                    .feasible = feasible};
}

/**
 * @brief 序列里程：PICK 出发，逐单元，最后回 END
 */
std::int64_t route_dist(const DistMatrix& matrix, const std::vector<int>& units) {
  std::int64_t total = 0;
  int prev = 0;
  for (const int unit : units) {
    total += matrix.get_dist(prev, unit);
    prev = unit;
  }
  return total + matrix.get_dist(prev, 5);
}

/**
 * @brief 把序列落到快照后重算里程
 */
std::int64_t dist_of(const DistMatrix& matrix, const std::vector<int>& units) {
  return route_dist(matrix, units);
}

}  // namespace

TEST(LsDistPruneTest, RelocateIntraShorterPasses) {
  const DistMatrix matrix = make_matrix();
  const LsDistPrune::DistanceTable table({make_route(&matrix, {3, 2, 1})});
  const LsMove move = LsRelocateMove{
      .block = {.load_ind = 0, .first_unit = 0, .unit_count = 1}, .to_load = 0, .to_slot = 2};

  const std::optional<std::int64_t> delta = LsDistPrune::distance_delta(move, table);

  ASSERT_TRUE(delta.has_value());
  EXPECT_EQ(*delta, dist_of(matrix, {2, 1, 3}) - dist_of(matrix, {3, 2, 1}));
  EXPECT_LT(*delta, 0);
  EXPECT_TRUE(LsDistPrune::passes(move, table));
}

TEST(LsDistPruneTest, RelocateIntraLongerIsPruned) {
  const DistMatrix matrix = make_matrix();
  const LsDistPrune::DistanceTable table({make_route(&matrix, {1, 2, 3})});
  const LsMove move = LsRelocateMove{
      .block = {.load_ind = 0, .first_unit = 0, .unit_count = 1}, .to_load = 0, .to_slot = 2};

  const std::optional<std::int64_t> delta = LsDistPrune::distance_delta(move, table);

  ASSERT_TRUE(delta.has_value());
  EXPECT_EQ(*delta, dist_of(matrix, {2, 3, 1}) - dist_of(matrix, {1, 2, 3}));
  EXPECT_GT(*delta, 0);
  EXPECT_FALSE(LsDistPrune::passes(move, table));
}

TEST(LsDistPruneTest, RelocateIntraNoOpHasZeroDeltaAndPasses) {
  const DistMatrix matrix = make_matrix();
  const LsDistPrune::DistanceTable table({make_route(&matrix, {1, 2, 3})});
  const LsMove move = LsRelocateMove{
      .block = {.load_ind = 0, .first_unit = 1, .unit_count = 1}, .to_load = 0, .to_slot = 1};

  const std::optional<std::int64_t> delta = LsDistPrune::distance_delta(move, table);

  ASSERT_TRUE(delta.has_value());
  EXPECT_EQ(*delta, 0);
  EXPECT_TRUE(LsDistPrune::passes(move, table));
}

TEST(LsDistPruneTest, RelocateCrossSumsBothRoutes) {
  const DistMatrix matrix = make_matrix();
  const LsDistPrune::DistanceTable table({make_route(&matrix, {1, 2}), make_route(&matrix, {4})});
  const LsMove move = LsRelocateMove{
      .block = {.load_ind = 0, .first_unit = 1, .unit_count = 1}, .to_load = 1, .to_slot = 0};

  const std::optional<std::int64_t> delta = LsDistPrune::distance_delta(move, table);

  ASSERT_TRUE(delta.has_value());
  const std::int64_t expected = (dist_of(matrix, {1}) + dist_of(matrix, {2, 4})) -
                                (dist_of(matrix, {1, 2}) + dist_of(matrix, {4}));
  EXPECT_EQ(*delta, expected);
  EXPECT_EQ(LsDistPrune::passes(move, table), expected <= 0);
}

TEST(LsDistPruneTest, ExchangeAdjacentSwapsBlocks) {
  const DistMatrix matrix = make_matrix();
  const LsDistPrune::DistanceTable table({make_route(&matrix, {1, 2, 3})});
  const LsMove move = LsExchangeMove{.first = {.load_ind = 0, .first_unit = 0, .unit_count = 1},
                                     .second = {.load_ind = 0, .first_unit = 1, .unit_count = 1}};

  const std::optional<std::int64_t> delta = LsDistPrune::distance_delta(move, table);

  ASSERT_TRUE(delta.has_value());
  EXPECT_EQ(*delta, dist_of(matrix, {2, 1, 3}) - dist_of(matrix, {1, 2, 3}));
  EXPECT_EQ(LsDistPrune::passes(move, table), *delta <= 0);
}

TEST(LsDistPruneTest, ExchangeNonAdjacentSwapsBlocks) {
  const DistMatrix matrix = make_matrix();
  const LsDistPrune::DistanceTable table({make_route(&matrix, {1, 2, 3, 4})});
  const LsMove move = LsExchangeMove{.first = {.load_ind = 0, .first_unit = 0, .unit_count = 1},
                                     .second = {.load_ind = 0, .first_unit = 3, .unit_count = 1}};

  const std::optional<std::int64_t> delta = LsDistPrune::distance_delta(move, table);

  ASSERT_TRUE(delta.has_value());
  EXPECT_EQ(*delta, dist_of(matrix, {4, 2, 3, 1}) - dist_of(matrix, {1, 2, 3, 4}));
}

TEST(LsDistPruneTest, ExchangeCrossSwapsBlocksBetweenRoutes) {
  const DistMatrix matrix = make_matrix();
  const LsDistPrune::DistanceTable table({make_route(&matrix, {1, 3}), make_route(&matrix, {4})});
  const LsMove move = LsExchangeMove{.first = {.load_ind = 0, .first_unit = 0, .unit_count = 1},
                                     .second = {.load_ind = 1, .first_unit = 0, .unit_count = 1}};

  const std::optional<std::int64_t> delta = LsDistPrune::distance_delta(move, table);

  ASSERT_TRUE(delta.has_value());
  const std::int64_t expected = (dist_of(matrix, {4, 3}) + dist_of(matrix, {1})) -
                                (dist_of(matrix, {1, 3}) + dist_of(matrix, {4}));
  EXPECT_EQ(*delta, expected);
}

TEST(LsDistPruneTest, ReverseTwoOptUsesReversedSequence) {
  const DistMatrix matrix = make_matrix();
  const LsDistPrune::DistanceTable table({make_route(&matrix, {1, 2, 3})});
  const LsMove move = LsReverseMove{.block = {.load_ind = 0, .first_unit = 0, .unit_count = 3}};

  const std::optional<std::int64_t> delta = LsDistPrune::distance_delta(move, table);

  ASSERT_TRUE(delta.has_value());
  EXPECT_EQ(*delta, dist_of(matrix, {3, 2, 1}) - dist_of(matrix, {1, 2, 3}));
  EXPECT_EQ(LsDistPrune::passes(move, table), *delta <= 0);
}

TEST(LsDistPruneTest, InfeasibleRouteIsNeverPruned) {
  const DistMatrix matrix = make_matrix();
  const LsDistPrune::DistanceTable table({make_route(&matrix, {1, 2, 3}, false)});
  const LsMove move = LsRelocateMove{
      .block = {.load_ind = 0, .first_unit = 0, .unit_count = 1}, .to_load = 0, .to_slot = 2};

  ASSERT_TRUE(LsDistPrune::distance_delta(move, table).has_value());
  EXPECT_GT(*LsDistPrune::distance_delta(move, table), 0);
  EXPECT_TRUE(LsDistPrune::passes(move, table));
}

TEST(LsDistPruneTest, MissingPickKeepsCandidate) {
  const DistMatrix matrix = make_matrix();
  LsDistPrune::RouteSnapshot target = make_route(&matrix, {4});
  target.pick_loc_ind = -1;
  const LsDistPrune::DistanceTable table({make_route(&matrix, {1, 2}), target});
  const LsMove move = LsRelocateMove{
      .block = {.load_ind = 0, .first_unit = 1, .unit_count = 1}, .to_load = 1, .to_slot = 0};

  EXPECT_FALSE(LsDistPrune::distance_delta(move, table).has_value());
  EXPECT_TRUE(LsDistPrune::passes(move, table));
}

TEST(LsDistPruneTest, OutOfRangeMoveKeepsCandidate) {
  const DistMatrix matrix = make_matrix();
  const LsDistPrune::DistanceTable table({make_route(&matrix, {1, 2})});
  const LsMove beyond =
      LsRelocateMove{.block = {.load_ind = 0, .first_unit = 1, .unit_count = 2}, .to_load = 0};
  const LsMove bad_slot = LsRelocateMove{
      .block = {.load_ind = 0, .first_unit = 0, .unit_count = 1}, .to_load = 0, .to_slot = 9};
  const LsMove bad_load = LsRelocateMove{.block = {.load_ind = 3, .first_unit = 0}, .to_load = 0};

  for (const LsMove& move : {beyond, bad_slot, bad_load}) {
    EXPECT_FALSE(LsDistPrune::distance_delta(move, table).has_value());
    EXPECT_TRUE(LsDistPrune::passes(move, table));
  }
}

TEST(LsDistPruneTest, EmptyTableKeepsCandidate) {
  const LsDistPrune::DistanceTable table;
  const LsMove move = LsRelocateMove{
      .block = {.load_ind = 0, .first_unit = 0, .unit_count = 1}, .to_load = 0, .to_slot = 1};

  EXPECT_TRUE(table.empty());
  EXPECT_TRUE(LsDistPrune::passes(move, table));
}
