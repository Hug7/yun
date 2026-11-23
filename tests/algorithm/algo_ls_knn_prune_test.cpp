/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#include "algo_ls_knn_prune.h"

#include <gtest/gtest.h>

#include <vector>

#include "algo_ls_move.h"

namespace {

// load0: 单元 0..3 分别承载订单 0..3；load1: 单元 0..1 分别承载订单 4、5
LsKnnPrune::UnitOrderTable make_table() {
  return LsKnnPrune::UnitOrderTable({{{0}, {1}, {2}, {3}}, {{4}, {5}}});
}

// 0-1、2-3、4-5 三条双向边
KnnGraph make_graph() {
  return KnnGraph{{0, {{1, 0}}}, {1, {{0, 0}}}, {2, {{3, 0}}},
                  {3, {{2, 0}}}, {4, {{5, 0}}}, {5, {{4, 0}}}};
}

LsBlock block(const int load_ind, const int first_unit, const int unit_count) {
  return LsBlock{.load_ind = load_ind, .first_unit = first_unit, .unit_count = unit_count};
}

}  // namespace

TEST(LsKnnPruneTest, IsKnnEdgeAcceptsEitherDirection) {
  const KnnGraph graph = make_graph();
  EXPECT_TRUE(LsKnnPrune::is_knn_edge(graph, 0, 1));
  EXPECT_TRUE(LsKnnPrune::is_knn_edge(graph, 1, 0));
  EXPECT_FALSE(LsKnnPrune::is_knn_edge(graph, 0, 2));
}

TEST(LsKnnPruneTest, IsKnnEdgeRejectsSelfAndNegative) {
  const KnnGraph graph = make_graph();
  EXPECT_FALSE(LsKnnPrune::is_knn_edge(graph, 0, 0));
  EXPECT_FALSE(LsKnnPrune::is_knn_edge(graph, -1, 0));
  EXPECT_FALSE(LsKnnPrune::is_knn_edge(graph, 0, -1));
  EXPECT_FALSE(LsKnnPrune::is_knn_edge(KnnGraph{}, 0, 1));
}

TEST(LsKnnPruneTest, UnitOrderTableHandlesOutOfRange) {
  const LsKnnPrune::UnitOrderTable table = make_table();
  EXPECT_EQ(table.unit_count(0), 4);
  EXPECT_EQ(table.unit_count(1), 2);
  EXPECT_EQ(table.unit_count(2), 0);
  EXPECT_EQ(table.unit_count(-1), 0);
  ASSERT_NE(table.unit_orders(0, 3), nullptr);
  EXPECT_EQ(*table.unit_orders(0, 3), std::vector<int>{3});
  EXPECT_EQ(table.unit_orders(0, 4), nullptr);
  EXPECT_EQ(table.unit_orders(2, 0), nullptr);
}

// 同车 relocate：源块从槽 0 搬到槽 2，新建 (单元2, 单元0) 与 (单元0, 单元3) 两对相邻
TEST(LsKnnPruneTest, RelocateIntraUsesSlotsAfterRemoval) {
  const LsKnnPrune::UnitOrderTable table = make_table();
  const LsMove move = LsRelocateMove{.block = block(0, 0, 1), .to_load = 0, .to_slot = 2};
  EXPECT_FALSE(LsKnnPrune::passes(move, table, make_graph()));

  KnnGraph with_edge = make_graph();
  with_edge[0][3] = 0;
  EXPECT_TRUE(LsKnnPrune::passes(move, table, with_edge));
}

// 同车 relocate 的槽位映射：槽位号不小于 first_unit 时跳过源块
TEST(LsKnnPruneTest, RelocateIntraMapsSlotsPastSourceBlock) {
  const LsKnnPrune::UnitOrderTable table = make_table();
  // 源块是槽 2，搬到剩余序列的槽 2 → 移除前下标 3，前驱映射为 1
  const LsMove move = LsRelocateMove{.block = block(0, 2, 1), .to_load = 0, .to_slot = 2};
  // 新建 (单元1, 单元2) 与 (单元2, 单元3)
  EXPECT_TRUE(LsKnnPrune::passes(move, table, make_graph()));

  // 搬到剩余序列的槽 0 → 只有一个后继相邻对 (单元2, 单元0)
  const LsMove to_front = LsRelocateMove{.block = block(0, 2, 1), .to_load = 0, .to_slot = 0};
  EXPECT_FALSE(LsKnnPrune::passes(to_front, table, make_graph()));
}

// 跨车 relocate：新建 (目标前驱, 块首) 与 (块尾, 目标后继)
TEST(LsKnnPruneTest, RelocateCrossChecksTargetNeighbours) {
  const LsKnnPrune::UnitOrderTable table = make_table();
  const LsMove move = LsRelocateMove{.block = block(0, 0, 1), .to_load = 1, .to_slot = 1};
  EXPECT_FALSE(LsKnnPrune::passes(move, table, make_graph()));

  KnnGraph with_edge = make_graph();
  with_edge[0][5] = 0;  // (块尾 订单0, 目标后继 订单5)
  EXPECT_TRUE(LsKnnPrune::passes(move, table, with_edge));

  // 搬到目标车最前 → 只剩后继一对
  const LsMove to_front = LsRelocateMove{.block = block(0, 0, 1), .to_load = 1, .to_slot = 0};
  EXPECT_FALSE(LsKnnPrune::passes(to_front, table, make_graph()));
  KnnGraph front_edge = make_graph();
  front_edge[0][4] = 0;  // (块尾 订单0, 目标后继 订单4)
  EXPECT_TRUE(LsKnnPrune::passes(to_front, table, front_edge));
}

// 同车 exchange：两块互换后检查各自的左右邻居
TEST(LsKnnPruneTest, ExchangeIntraChecksBothSides) {
  const LsKnnPrune::UnitOrderTable table = make_table();
  const LsMove move = LsExchangeMove{.first = block(0, 0, 1), .second = block(0, 2, 1)};
  // 新建 (订单1, 订单0)、(订单2, 订单1)、(订单0, 订单3)：这里只给交叉边 0-2、1-3
  KnnGraph graph;
  graph[0][2] = 0;
  graph[1][3] = 0;
  EXPECT_FALSE(LsKnnPrune::passes(move, table, graph));

  graph[1][0] = 0;
  EXPECT_TRUE(LsKnnPrune::passes(move, table, graph));
}

TEST(LsKnnPruneTest, ExchangeCrossChecksTargetNeighbours) {
  const LsKnnPrune::UnitOrderTable table = make_table();
  const LsMove move = LsExchangeMove{.first = block(0, 0, 1), .second = block(1, 0, 1)};
  // 新建 (订单4, 订单1) 与 (订单0, 订单5)
  KnnGraph with_edge = make_graph();
  with_edge[4][1] = 0;
  EXPECT_TRUE(LsKnnPrune::passes(move, table, with_edge));
  EXPECT_FALSE(LsKnnPrune::passes(move, table, make_graph()));
}

// 反转：段尾接到段前，段首接到段后
TEST(LsKnnPruneTest, ReverseChecksBothEnds) {
  const LsKnnPrune::UnitOrderTable table = make_table();
  // 反转槽 1..2 → (订单0, 订单2) 与 (订单1, 订单3)
  const LsMove inner = LsReverseMove{.block = block(0, 1, 2)};
  EXPECT_FALSE(LsKnnPrune::passes(inner, table, make_graph()));

  KnnGraph with_edge = make_graph();
  with_edge[1][3] = 0;
  EXPECT_TRUE(LsKnnPrune::passes(inner, table, with_edge));

  // 反转整个车 → 段前段后都不存在，没有相邻对
  const LsMove whole = LsReverseMove{.block = block(0, 0, 4)};
  EXPECT_TRUE(LsKnnPrune::passes(whole, table, make_graph()));
}

// 同车两块紧邻时，互换后两块彼此相邻，这对新邻居也要检查
TEST(LsKnnPruneTest, AdjacentExchangeChecksBlockPair) {
  const LsKnnPrune::UnitOrderTable table = make_table();
  const LsMove move = LsExchangeMove{.first = block(0, 0, 1), .second = block(0, 1, 1)};
  // 新建 (订单1, 订单0) 与 (订单0, 订单2)
  EXPECT_TRUE(LsKnnPrune::passes(move, table, make_graph()));
  EXPECT_FALSE(LsKnnPrune::passes(move, table, KnnGraph{}));
}

// 只有单个单元的车次取不到相邻对，不能误剪
TEST(LsKnnPruneTest, SingleUnitLoadIsNeverPruned) {
  const LsKnnPrune::UnitOrderTable table = LsKnnPrune::UnitOrderTable({{{0}}, {{1}}});
  const LsMove reverse = LsReverseMove{.block = block(0, 0, 1)};
  EXPECT_TRUE(LsKnnPrune::passes(reverse, table, KnnGraph{}));
}

// 单元承载多个订单：任意一对构成 KNN 边即通过
TEST(LsKnnPruneTest, MultiOrderUnitMatchesAnyPair) {
  const LsKnnPrune::UnitOrderTable table = LsKnnPrune::UnitOrderTable({{{0, 7}, {1}, {2}}, {{4}}});
  const LsMove move = LsReverseMove{.block = block(0, 0, 2)};
  // 反转整段后段尾接到单元 2，候选相邻对是 ({0, 7}, {2})
  KnnGraph graph;
  graph[1][2] = 0;  // 订单 1 不在该单元里，不算相邻
  EXPECT_FALSE(LsKnnPrune::passes(move, table, graph));
  graph[7][2] = 0;
  EXPECT_TRUE(LsKnnPrune::passes(move, table, graph));
}
