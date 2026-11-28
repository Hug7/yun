/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>

#include <algorithm>
#include <cstddef>
#include <functional>
#include <memory>
#include <stdexcept>
#include <utility>
#include <vector>

#include "algo_alns_repair_fixture.h"
#include "algo_alns_repair_greedy.h"
#include "algo_alns_repair_regret_k.h"
#include "algo_parallel_executor.h"
#include "algo_segment_builder.h"
#include "pdm_resource.h"
#include "pdm_solution.h"
#include "prob_load_editor.h"

namespace {
using CommitSeq = std::vector<RepairCellView>;

/**
 * @brief 一次 repair 运行的可观测结果
 */
struct RepairRun {
  CommitSeq commits;
  double obj_val{0.0};
  double cost{0.0};
  double penalty{0.0};
  std::size_t load_count{0};
  std::size_t unassigned_count{0};
};

/**
 * @brief 用真实 RepairMatrix + 真实算子跑一次 repair
 * @details 单线程执行器固定矩阵构造与提交顺序; commit_trace 记录每次提交的
 * (segment_ind, load_ind), 用于确定性与黄金值比对
 */
RepairRun run_repair(const AlnsRepairFixture& fixture,
                     const std::function<void(RepairMatrix*, ParallelExecutor*)>& execute) {
  std::unique_ptr<Solution> sol = fixture.make_solution();
  SegmentBuilder segment_builder;
  std::vector<Segment> segments = segment_builder.call(sol.get());
  EXPECT_EQ(static_cast<int>(segments.size()), AlnsRepairFixture::unassigned_order_count());

  ParallelExecutor executor(1);
  RepairMatrix matrix(&executor, fixture.load_editor(), fixture.load_context(), sol.get(),
                      std::move(segments));
  matrix.init();

  RepairRun run;
  matrix.commit_trace = &run.commits;
  execute(&matrix, &executor);

  run.obj_val = sol->total_obj_val();
  run.cost = sol->total_cost();
  run.penalty = sol->total_penalty();
  run.load_count = sol->loads.size();
  run.unassigned_count = sol->unassigned_orders.size();
  return run;
}

RepairRun run_greedy(const AlnsRepairFixture& fixture) {
  return run_repair(fixture, [](RepairMatrix* matrix, ParallelExecutor* executor) {
    AlnsRepairGreedy op(nullptr, executor, AlnsRepairGreedyParam{37, 0.2});
    op.execute(matrix);
  });
}

RepairRun run_regret_k(const AlnsRepairFixture& fixture) {
  return run_repair(fixture, [](RepairMatrix* matrix, ParallelExecutor* executor) {
    AlnsRepairRegretK op(nullptr, executor, AlnsRepairRegretKParam{2});
    op.execute(matrix);
  });
}

/**
 * @brief 断言提交序列与黄金值逐项相等
 */
void expect_commit_seq_eq(const CommitSeq& actual,
                          const std::vector<std::pair<int, int>>& expected) {
  ASSERT_EQ(actual.size(), expected.size());
  for (std::size_t i = 0; i < expected.size(); ++i) {
    EXPECT_EQ(actual[i].segment_ind, expected[i].first) << "commit index=" << i;
    EXPECT_EQ(actual[i].load_ind, expected[i].second) << "commit index=" << i;
  }
}

/**
 * @brief 断言一次运行与黄金值一致
 */
void expect_view_seq_eq(const CommitSeq& actual, const CommitSeq& expected) {
  ASSERT_EQ(actual.size(), expected.size());
  for (std::size_t i = 0; i < expected.size(); ++i) {
    EXPECT_EQ(actual[i].delta, expected[i].delta) << "commit index=" << i;
    EXPECT_EQ(actual[i].segment_ind, expected[i].segment_ind) << "commit index=" << i;
    EXPECT_EQ(actual[i].load_ind, expected[i].load_ind) << "commit index=" << i;
  }
}

/**
 * @brief 断言一次运行与黄金值一致
 */
void expect_run_eq(const RepairRun& actual,
                   const std::vector<std::pair<int, int>>& expected_commits,
                   const double expected_obj_val, const std::size_t expected_load_count) {
  expect_commit_seq_eq(actual.commits, expected_commits);
  EXPECT_DOUBLE_EQ(actual.obj_val, expected_obj_val);
  EXPECT_EQ(actual.load_count, expected_load_count);
  EXPECT_EQ(actual.unassigned_count, 0U);
}
}  // namespace

/**
 * @brief 黄金值记录自 pre-P0 基线(HEAD 71e60d9 + 行为中性的测试构造重载/commit_trace 钩子),
 * 与 P0 的表示重构无关: 同一 seed 下 greedy 的 (segment_ind, load_ind) 提交序列必须逐项一致
 */
TEST(AlgoAlnsRepairMatrixTest, GreedyMatchesPreP0Golden) {
  AlnsRepairFixture fixture;
  const RepairRun run = run_greedy(fixture);
  // 黄金值(pre-P0 基线记录): 3 个 segment 依次提交到 load 0, 最终目标值 266
  expect_run_eq(run, {{0, 0}, {1, 0}, {2, 0}}, 266.0, 2U);
}

/**
 * @brief regret-k 的提交序列同样必须与 pre-P0 黄金值逐项一致
 */
TEST(AlgoAlnsRepairMatrixTest, RegretKMatchesPreP0Golden) {
  AlnsRepairFixture fixture;
  const RepairRun run = run_regret_k(fixture);
  // 黄金值(pre-P0 基线记录): 与 greedy 相同的提交序列与最终目标值
  expect_run_eq(run, {{0, 0}, {1, 0}, {2, 0}}, 266.0, 2U);
}

/**
 * @brief 同一 seed 下重复运行必须给出完全相同的提交序列与目标值
 */
TEST(AlgoAlnsRepairMatrixTest, SameSeedIsDeterministic) {
  AlnsRepairFixture fixture;
  const RepairRun first = run_greedy(fixture);
  const RepairRun second = run_greedy(fixture);
  expect_view_seq_eq(second.commits, first.commits);
  EXPECT_EQ(second.obj_val, first.obj_val);
  EXPECT_EQ(second.load_count, first.load_count);
}

/**
 * @brief P0 表示层不变式: 行稠密且长度等于当前列数, idle 按提交顺序收缩,
 * 提交序列每个 (segment, load) 只出现一次, 运行后不再有未指派订单
 */
TEST(AlgoAlnsRepairMatrixTest, MatrixInvariantsAfterGreedy) {
  AlnsRepairFixture fixture;
  std::unique_ptr<Solution> sol = fixture.make_solution();
  SegmentBuilder segment_builder;
  std::vector<Segment> segments = segment_builder.call(sol.get());
  const int initial_load_count = static_cast<int>(sol->loads.size());

  ParallelExecutor executor(1);
  RepairMatrix matrix(&executor, fixture.load_editor(), fixture.load_context(), sol.get(),
                      std::move(segments));
  EXPECT_EQ(static_cast<int>(matrix.idle_segment_indices.size()),
            AlnsRepairFixture::unassigned_order_count());
  matrix.init();
  ASSERT_EQ(static_cast<int>(matrix.rows.size()), AlnsRepairFixture::unassigned_order_count());
  for (const auto& row : matrix.rows) {
    EXPECT_TRUE(row.active);
    EXPECT_EQ(static_cast<int>(row.cells.size()), initial_load_count);
    // P2-A: 行内有序视图必须与 cells 的可行格一一对应且升序
    int feasible_cells = 0;
    for (const auto& cell : row.cells) {
      feasible_cells += cell.is_feasible ? 1 : 0;
    }
    EXPECT_EQ(static_cast<int>(row.ranked.size()), feasible_cells);
    EXPECT_TRUE(std::is_sorted(row.ranked.begin(), row.ranked.end(), RankedCellLess{}));
  }

  CommitSeq commits;
  matrix.commit_trace = &commits;
  AlnsRepairGreedy op(nullptr, &executor, AlnsRepairGreedyParam{37, 0.2});
  op.execute(&matrix);

  EXPECT_FALSE(commits.empty());
  // (segment, load) 组合不可重复提交
  std::vector<std::pair<int, int>> sorted;
  sorted.reserve(commits.size());
  for (const auto& view : commits) {
    sorted.emplace_back(view.segment_ind, view.load_ind);
  }
  std::sort(sorted.begin(), sorted.end());
  EXPECT_EQ(std::unique(sorted.begin(), sorted.end()), sorted.end());
  // 每个 segment 最多提交一次
  std::vector<int> committed_segments;
  committed_segments.reserve(commits.size());
  for (const auto& view : commits) {
    EXPECT_GE(view.segment_ind, 0);
    EXPECT_LT(view.segment_ind, AlnsRepairFixture::unassigned_order_count());
    committed_segments.push_back(view.segment_ind);
  }
  std::sort(committed_segments.begin(), committed_segments.end());
  EXPECT_EQ(std::unique(committed_segments.begin(), committed_segments.end()),
            committed_segments.end());
  // 全部未指派订单都已被消费
  EXPECT_EQ(sol->unassigned_orders.size(), 0U);
  EXPECT_EQ(matrix.unfold_matrix_cell_count(), 0);
}

/**
 * @brief 车型占满即刷新: 某车型被占满后, 仍在记录它的格子会被立即重评估
 * @details 取代 P1 的提交守卫慢路径(见 RepairMatrix::sync_vehicle_fullness): 任一可行格
 * 要么记录的车型就是该 load 的当前车型(Solution::update_load 先释放后占用, 净零),
 * 要么该车型仍有空位。夹具的初始解占满 V0、V1, V2 是唯一空位; 这里白盒地把一格写成
 * "计划换到 V2"(模拟评估之后 V2 被他人占满的时序), 再让 matrix 自己新建车次占满 V2,
 * 断言该格已被重评估、且全表不再有记录不可用车型的格子。
 */
TEST(AlgoAlnsRepairMatrixTest, SaturatedVehicleRefreshesAffectedCells) {
  AlnsRepairFixture fixture;
  std::unique_ptr<Solution> sol = fixture.make_solution();
  SegmentBuilder segment_builder;
  std::vector<Segment> segments = segment_builder.call(sol.get());

  ParallelExecutor executor(1);
  RepairMatrix matrix(&executor, fixture.load_editor(), fixture.load_context(), sol.get(),
                      std::move(segments));
  matrix.init();

  const auto& vehicle_usages = sol->vehicle_resource->vehicle_usages;
  ASSERT_FALSE(vehicle_usages[0]->has_available_vehicle());
  ASSERT_FALSE(vehicle_usages[1]->has_available_vehicle());
  ASSERT_TRUE(vehicle_usages[2]->has_available_vehicle());

  // 挑一格不在"将被新建车次消费的 segment"上的可行格(该行之后仍然有效)
  const int consumed_segment = matrix.idle_segment_indices.front();
  int target_segment = -1;
  int target_load = -1;
  for (const int s : matrix.idle_segment_indices) {
    if (s == consumed_segment) {
      continue;
    }
    for (std::size_t l = 0; l < matrix.rows[static_cast<std::size_t>(s)].cells.size(); ++l) {
      if (matrix.rows[static_cast<std::size_t>(s)].cells[l].is_feasible &&
          sol->loads[l]->vehicle->ind != 2) {
        target_segment = s;
        target_load = static_cast<int>(l);
        break;
      }
    }
    if (target_segment >= 0) {
      break;
    }
  }
  ASSERT_GE(target_segment, 0);
  RepairCell& target = matrix.rows[static_cast<std::size_t>(target_segment)]
                           .cells[static_cast<std::size_t>(target_load)];
  target.vehicle_ind = 2;

  // 真实触发占满: 新建车次只能落在 V2 上
  ASSERT_TRUE(matrix.idle_segment_create_load());
  ASSERT_FALSE(vehicle_usages[2]->has_available_vehicle());

  // 该格已被重评估: 不再记录已经满仓的 V2
  const RepairCell& refreshed = matrix.rows[static_cast<std::size_t>(target_segment)]
                                    .cells[static_cast<std::size_t>(target_load)];
  EXPECT_NE(refreshed.vehicle_ind, 2);

  // 不变量: 可行格记录的非原车型, 必须仍能再占用一次
  int stale = 0;
  for (std::size_t s = 0; s < matrix.rows.size(); ++s) {
    const RepairRow& row = matrix.rows[s];
    if (!row.active) {
      continue;
    }
    for (std::size_t l = 0; l < row.cells.size(); ++l) {
      const RepairCell& cell = row.cells[l];
      if (!cell.is_feasible || cell.vehicle_ind == sol->loads[l]->vehicle->ind) {
        continue;
      }
      ++stale;
      EXPECT_TRUE(sol->vehicle_resource->has_available(
          vehicle_usages[static_cast<std::size_t>(cell.vehicle_ind)]->vehicle))
          << "cell(" << s << "," << l << ") 记录了已无空位的车型 " << cell.vehicle_ind;
    }
  }
  EXPECT_EQ(stale, 0);
}

/**
 * @brief 车型快照按成本升序给出全部车型, 并标出哪些还有空位
 * @details 初始解已占满 V0/V1, 只有 V2 有空位; 快照里三个车型成本相同, 稳定排序保持
 * vehicle_usages 顺序, 选车应落到唯一还有空位的 V2
 */
TEST(AlgoAlnsRepairMatrixTest, SnapshotVehiclesReportsCostAndResource) {
  AlnsRepairFixture fixture;
  std::unique_ptr<Solution> sol = fixture.make_solution();
  ASSERT_FALSE(sol->vehicle_resource->vehicle_usages[0]->has_available_vehicle());
  ASSERT_FALSE(sol->vehicle_resource->vehicle_usages[1]->has_available_vehicle());

  const std::vector<const Order*> orders{fixture.orders()[2]};
  Load* load = fixture.load_editor()->build(orders, *sol->vehicle_resource);
  ASSERT_NE(load, nullptr);
  ASSERT_TRUE(load->is_feasible());

  const std::vector<VehicleInsertSnapshot> snapshots =
      fixture.load_editor()->snapshot_vehicles(load, *sol->vehicle_resource);
  ASSERT_EQ(snapshots.size(), 3U);
  for (std::size_t i = 1; i < snapshots.size(); ++i) {
    EXPECT_LE(snapshots[i - 1].obj_val, snapshots[i].obj_val);
  }
  for (const VehicleInsertSnapshot& snapshot : snapshots) {
    // V0/V1 已被初始车次占满, 只有 V2 还有空位
    EXPECT_EQ(snapshot.has_resource, snapshot.vehicle->ind == 2);
  }

  const Vehicle* chosen =
      fixture.load_editor()->select_current_best_vehicle(load, *sol->vehicle_resource);
  ASSERT_NE(chosen, nullptr);
  EXPECT_EQ(chosen->ind, 2);
  EXPECT_TRUE(load->is_feasible());
  delete load;
}

/**
 * @brief 车型空出后, 评估时把它列为待跟踪车型的格子会被重评估
 * @details 夹具里三个车型能力相同, 自然轨迹不会产生容量差异, 因此白盒构造: 先新建车次占满
 * V2, 再把一格写成不可行并把 V2 记进 watched_vehicles 位图, 随后模拟换车释放 V2,
 * 提交另一行、另一列的可行格触发 sync_vehicle_fullness。没有 retry_released_vehicle
 * 时这一格会一直不可行, 直到该列被重算, 当时本可用的候选就白白丢了。
 */
TEST(AlgoAlnsRepairMatrixTest, FreedVehicleRetriesWatchedCells) {
  AlnsRepairFixture fixture;
  std::unique_ptr<Solution> sol = fixture.make_solution();
  SegmentBuilder segment_builder;
  std::vector<Segment> segments = segment_builder.call(sol.get());

  ParallelExecutor executor(1);
  RepairMatrix matrix(&executor, fixture.load_editor(), fixture.load_context(), sol.get(),
                      std::move(segments));
  matrix.init();

  const auto& vehicle_usages = sol->vehicle_resource->vehicle_usages;
  ASSERT_TRUE(vehicle_usages[2]->has_available_vehicle());

  // 先真实占满 V2(新建车次只能落在 V2), 之后才有"空出"这一次跃迁
  const int consumed_segment = matrix.idle_segment_indices.front();
  ASSERT_TRUE(matrix.idle_segment_create_load());
  ASSERT_FALSE(vehicle_usages[2]->has_available_vehicle());

  // 挑一格不在被消费行上的可行格, 打成不可行并把它曾跳过的 V2 记进位图
  int target_segment = -1;
  int target_load = -1;
  for (const int s : matrix.idle_segment_indices) {
    if (s == consumed_segment) {
      continue;
    }
    for (std::size_t l = 0; l < matrix.rows[static_cast<std::size_t>(s)].cells.size(); ++l) {
      if (matrix.rows[static_cast<std::size_t>(s)].cells[l].is_feasible &&
          sol->loads[l]->vehicle->ind != 2) {
        target_segment = s;
        target_load = static_cast<int>(l);
        break;
      }
    }
    if (target_segment >= 0) {
      break;
    }
  }
  ASSERT_GE(target_segment, 0);
  RepairCell& target = matrix.rows[static_cast<std::size_t>(target_segment)]
                           .cells[static_cast<std::size_t>(target_load)];
  ASSERT_TRUE(target.is_feasible);
  target = RepairCell{};
  target.watched_vehicles = 1ULL << 2;

  // 模拟换车释放 V2
  vehicle_usages[2]->release_vehicle();
  ASSERT_TRUE(vehicle_usages[2]->has_available_vehicle());

  // 提交另一行、另一列的可行格(记录本 load 当前车型, 占用净零)触发 sync
  int trigger_segment = -1;
  int trigger_load = -1;
  double trigger_delta = 0.0;
  for (const int s : matrix.idle_segment_indices) {
    if (s == target_segment) {
      continue;
    }
    for (std::size_t l = 0; l < matrix.rows[static_cast<std::size_t>(s)].cells.size(); ++l) {
      // 必须换一列: 提交会重算该列, 否则目标格会被列重算顺带修好, 用例就测不到重试
      if (static_cast<int>(l) == target_load) {
        continue;
      }
      const RepairCell& cell = matrix.rows[static_cast<std::size_t>(s)].cells[l];
      if (cell.is_feasible && cell.vehicle_ind == sol->loads[l]->vehicle->ind) {
        trigger_segment = s;
        trigger_load = static_cast<int>(l);
        trigger_delta = cell.delta;
        break;
      }
    }
    if (trigger_segment >= 0) {
      break;
    }
  }
  ASSERT_GE(trigger_segment, 0);
  matrix.commit(RepairCellView{trigger_delta, trigger_segment, trigger_load});

  // 该格已被重评估: 重新可行, 且位图被新的评估结果覆盖(不再等 V2)
  const RepairCell& retried = matrix.rows[static_cast<std::size_t>(target_segment)]
                                  .cells[static_cast<std::size_t>(target_load)];
  EXPECT_TRUE(retried.is_feasible);
  EXPECT_EQ(retried.watched_vehicles, 0);
  EXPECT_NE(retried.vehicle_ind, 2);

  // 定点重评估后 O(1) 计数仍与全表重算一致
  int feasible_total = 0;
  for (const RepairRow& row : matrix.rows) {
    if (row.active) {
      feasible_total += static_cast<int>(row.ranked.size());
    }
  }
  EXPECT_EQ(matrix.unfold_matrix_cell_count(), feasible_total);
}

/**
 * @brief 候选与解不同构时 commit 记录日志并抛 std::logic_error, 不静默降级
 * @details 白盒把可行格的槽位改成一个不可能枚举出来的值, 版本闸门与资源不变式仍然通过,
 * 于是 apply_insert 重放失败。旧实现会返回 false 让调用方 mark_infeasible, 把一个仍然
 * 可行的候选永久丢掉; 现在要求直接失败, 且失败前不改动矩阵状态。
 */
TEST(AlgoAlnsRepairMatrixTest, CommitThrowsWhenPlanNoLongerApplies) {
  AlnsRepairFixture fixture;
  std::unique_ptr<Solution> sol = fixture.make_solution();
  SegmentBuilder segment_builder;
  std::vector<Segment> segments = segment_builder.call(sol.get());

  ParallelExecutor executor(1);
  RepairMatrix matrix(&executor, fixture.load_editor(), fixture.load_context(), sol.get(),
                      std::move(segments));
  matrix.init();

  const std::vector<RepairCellView> views = matrix.unfold_matrix_top_k(1);
  ASSERT_FALSE(views.empty());
  const RepairCellView view = views.front();
  RepairCell& cell = matrix.rows[static_cast<std::size_t>(view.segment_ind)]
                         .cells[static_cast<std::size_t>(view.load_ind)];
  ASSERT_TRUE(cell.is_feasible);
  const std::int32_t saved_slot = cell.slot_ind;
  const int feasible_before = matrix.unfold_matrix_cell_count();
  cell.slot_ind = 1'000'000;

  EXPECT_THROW(matrix.commit(view), std::logic_error);

  // 失败不修改矩阵: 格子、计数、行状态都不变
  EXPECT_TRUE(cell.is_feasible);
  EXPECT_EQ(matrix.unfold_matrix_cell_count(), feasible_before);
  EXPECT_TRUE(matrix.rows[static_cast<std::size_t>(view.segment_ind)].active);

  // 恢复槽位后同一候选可以正常落实
  cell.slot_ind = saved_slot;
  matrix.commit(view);
  EXPECT_FALSE(matrix.rows[static_cast<std::size_t>(view.segment_ind)].active);
}

/**
 * @brief P1 失效标记: mark_infeasible 把候选置为不可行, 可行格计数单调下降,
 * 后续展开不再返回该格(保证算子循环收敛)
 */
TEST(AlgoAlnsRepairMatrixTest, MarkInfeasibleShrinksFeasibleCount) {
  AlnsRepairFixture fixture;
  std::unique_ptr<Solution> sol = fixture.make_solution();
  SegmentBuilder segment_builder;
  std::vector<Segment> segments = segment_builder.call(sol.get());

  ParallelExecutor executor(1);
  RepairMatrix matrix(&executor, fixture.load_editor(), fixture.load_context(), sol.get(),
                      std::move(segments));
  matrix.init();
  const int feasible_before = matrix.unfold_matrix_cell_count();
  ASSERT_GT(feasible_before, 0);

  const std::vector<RepairCellView> views = matrix.unfold_matrix_top_k(1);
  ASSERT_FALSE(views.empty());
  const RepairCellView select_view = views.front();
  matrix.mark_infeasible(select_view);
  EXPECT_EQ(matrix.unfold_matrix_cell_count(), feasible_before - 1);

  const std::vector<RepairCellView> remaining = matrix.unfold_matrix_top_k(1);
  ASSERT_FALSE(remaining.empty());
  EXPECT_FALSE(remaining.front().segment_ind == select_view.segment_ind &&
               remaining.front().load_ind == select_view.load_ind);
}

/**
 * @brief P1 计划落实的失败路径: 槽位越界的计划不物化 Load, 直接返回 nullptr
 */
TEST(AlgoAlnsRepairMatrixTest, ApplyInsertRejectsOutOfRangeSlot) {
  AlnsRepairFixture fixture;
  std::unique_ptr<Solution> sol = fixture.make_solution();
  SegmentBuilder segment_builder;
  std::vector<Segment> segments = segment_builder.call(sol.get());
  ASSERT_FALSE(segments.empty());
  const InsertPlan plan{-1, nullptr, 0.0};
  EXPECT_EQ(fixture.load_editor()->apply_insert(sol->loads[0], &segments[0], plan, nullptr),
            nullptr);
}
/**
 * @brief P2-A 增量计数: 每次初始化/失效/提交/列重算后, O(1) 可行格计数必须与全表重算一致
 */
TEST(AlgoAlnsRepairMatrixTest, IncrementalFeasibleCountMatchesFullRecount) {
  AlnsRepairFixture fixture;
  std::unique_ptr<Solution> sol = fixture.make_solution();
  SegmentBuilder segment_builder;
  std::vector<Segment> segments = segment_builder.call(sol.get());

  ParallelExecutor executor(1);
  RepairMatrix matrix(&executor, fixture.load_editor(), fixture.load_context(), sol.get(),
                      std::move(segments));
  matrix.init();
  auto full_feasible_count = [](const RepairMatrix& target) {
    int total = 0;
    for (const RepairRow& row : target.rows) {
      if (!row.active) {
        continue;
      }
      for (const RepairCell& cell : row.cells) {
        total += cell.is_feasible ? 1 : 0;
      }
    }
    return total;
  };
  EXPECT_EQ(matrix.unfold_matrix_cell_count(), full_feasible_count(matrix));

  // mark_infeasible: 精确减一, 与全表重算一致
  const std::vector<RepairCellView> best = matrix.unfold_matrix_top_k(1);
  ASSERT_FALSE(best.empty());
  const int before_mark = matrix.unfold_matrix_cell_count();
  matrix.mark_infeasible(best.front());
  EXPECT_EQ(matrix.unfold_matrix_cell_count(), before_mark - 1);
  EXPECT_EQ(matrix.unfold_matrix_cell_count(), full_feasible_count(matrix));

  // commit: 移除整行 + 重算一列, 两处都要同步维护计数
  const std::vector<RepairCellView> next = matrix.unfold_matrix_top_k(1);
  ASSERT_FALSE(next.empty());
  matrix.commit(next.front());
  EXPECT_EQ(matrix.unfold_matrix_cell_count(), full_feasible_count(matrix));

  // 剩余 segment 继续 repair 到收敛: 计数随行移除归零
  AlnsRepairGreedy op(nullptr, &executor, AlnsRepairGreedyParam{37, 0.2});
  op.execute(&matrix);
  EXPECT_EQ(matrix.unfold_matrix_cell_count(), full_feasible_count(matrix));
  EXPECT_EQ(matrix.unfold_matrix_cell_count(), 0);
}
