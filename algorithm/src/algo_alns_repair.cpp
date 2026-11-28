/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#include "algo_alns_repair.h"

#include <spdlog/spdlog.h>

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <optional>
#include <stdexcept>
#include <utility>

#include "algo_alns_repair_rank.h"
#include "c_rang_utils.h"
#include "pdm_context.h"
#include "prob_load_editor.h"

namespace {
/**
 * @brief 把可行格插入行内有序视图(保持 (delta, load_ind) 全序)
 */
void insert_ranked(std::vector<RankedCell>& ranked, const double delta, const int load_ind) {
  const RankedCell item{.delta = delta, .load_ind = load_ind};
  ranked.insert(std::ranges::lower_bound(ranked, item, RankedCellLess{}), item);
}

/**
 * @brief 从行内有序视图删除指定格
 */
void erase_ranked(std::vector<RankedCell>& ranked, const double delta, const int load_ind) {
  const RankedCell key{.delta = delta, .load_ind = load_ind};
  const auto it = std::ranges::lower_bound(ranked, key, RankedCellLess{});
  assert(it != ranked.end());
  assert(it->delta == delta);
  assert(it->load_ind == load_ind);
  ranked.erase(it);
}

/**
 * @brief 由 cells 全量重建行内有序视图
 */
void rebuild_ranked(RepairRow& row) {
  row.ranked.clear();
  row.ranked.reserve(row.cells.size());
  for (std::size_t load_ind = 0; load_ind < row.cells.size(); ++load_ind) {
    const RepairCell& cell = row.cells[load_ind];
    if (cell.is_feasible) {
      row.ranked.push_back(RankedCell{.delta = cell.delta, .load_ind = static_cast<int>(load_ind)});
    }
  }
  std::ranges::sort(row.ranked, RankedCellLess{});
}
}  // namespace

bool RankedCellLess::operator()(const RankedCell& lhs, const RankedCell& rhs) const {
  if (lhs.delta < rhs.delta) {
    return true;
  }
  if (rhs.delta < lhs.delta) {
    return false;
  }
  return lhs.load_ind < rhs.load_ind;
}

// ====== implement of RepairMatrix ======
RepairMatrix::RepairMatrix(ParallelExecutor* _executor, const SolverContext* _context,
                           Solution* _sol, std::vector<Segment> _segments)
    : RepairMatrix(_executor, _context->load_editor, _context->problem->load_context, _sol,
                   std::move(_segments)) {
  this->context = _context;
}

RepairMatrix::RepairMatrix(ParallelExecutor* _executor, LoadEditor* _load_editor,
                           LoadContext* _load_context, Solution* _sol,
                           std::vector<Segment> _segments)
    : parallel_executor(_executor),
      load_editor(_load_editor),
      load_context(_load_context),
      sol(_sol),
      segments(std::move(_segments)) {
  this->idle_segment_indices = RangeUtils::range_vector(static_cast<int>(this->segments.size()));
}

void RepairMatrix::run_parallel(const int task_count,
                                const std::function<void(int, CacheStoreBuffer*)>& task) const {
  if (task_count <= 0) {
    return;
  }
  // 归档内置在并行入口中: run 返回前所有写缓冲都已并入冻结表
  this->parallel_executor->run(
      task_count, this->load_context, [&](const int u, CacheStoreBuffer* store_buffer) {
        try {
          task(u, store_buffer);
        } catch (const std::exception& e) {
          // 异常一律收敛在任务体内, 不穿越并行边界: 单个 cell 算不出来不应中断整个 repair
          this->context->logger->warn("repair task {} 失败, 按不可行处理: {}", u, e.what());
        } catch (...) {
          this->context->logger->warn("repair task {} 抛出未知异常, 按不可行处理", u);
        }
      });
}

RepairCell RepairMatrix::eval_cell(const int segment_ind, const int load_ind,
                                   CacheStoreBuffer* store_buffer) const {
  RepairCell cell;
  std::uint64_t watched = 0;
  // 只评估计划, 不物化 Load: 常驻内存不再随候选数增长
  const std::optional<InsertPlan> plan =
      this->load_editor->evaluate_insert(this->sol->loads[static_cast<std::size_t>(load_ind)],
                                         &this->segments[static_cast<std::size_t>(segment_ind)],
                                         *this->sol->vehicle_resource, store_buffer, &watched);
  if (plan.has_value()) {
    cell.delta =
        plan->obj_val - this->sol->loads[static_cast<std::size_t>(load_ind)]->get_obj_val();
    cell.slot_ind = static_cast<std::int32_t>(plan->slot_ind);
    // evaluate_insert 在无可用车型时返回 nullopt, 有 plan 就一定有车型
    assert(plan->vehicle != nullptr);
    cell.vehicle_ind = static_cast<std::int32_t>(plan->vehicle->ind);
    cell.is_feasible = true;
  }
  // 位图由选车过程写下: 可行格记"还有更便宜但被占满的车型", 不可行格记"哪些车型一释放就可能可行"
  cell.watched_vehicles = watched;
  // 版本断言: 格版本必须等于所属列的当前版本
  cell.version = this->load_version_[static_cast<std::size_t>(load_ind)];
  return cell;
}

void RepairMatrix::init() {
  const int segment_count = static_cast<int>(this->segments.size());
  const int load_count = static_cast<int>(this->sol->loads.size());

  // 串行预建全部行: 并行阶段只允许改写各自的元素, 任何插入/扩容都是未定义行为
  this->rows.clear();
  this->rows.resize(static_cast<std::size_t>(segment_count));
  this->idle_segment_indices = RangeUtils::range_vector(segment_count);
  this->load_version_.assign(static_cast<std::size_t>(load_count), 0);
  this->version_counter_ = 0;
  for (int s = 0; s < segment_count; ++s) {
    this->rows[static_cast<std::size_t>(s)].active = true;
    this->rows[static_cast<std::size_t>(s)].cells.resize(static_cast<std::size_t>(load_count));
  }

  // 并行阶段: 每个 (segment, load) 一格, 行主序动态领取
  this->run_parallel(segment_count * load_count, [&](const int u, CacheStoreBuffer* store_buffer) {
    const int s = u / load_count;
    const int l = u % load_count;
    this->rows[static_cast<std::size_t>(s)].cells[static_cast<std::size_t>(l)] =
        this->eval_cell(s, l, store_buffer);
  });
  // 串行阶段构建每行的有序可行视图
  this->feasible_count_ = 0;
  for (RepairRow& row : this->rows) {
    if (!row.active) {
      continue;
    }
    // 构建行内有序视图
    rebuild_ranked(row);
    this->feasible_count_ += static_cast<int>(row.ranked.size());
  }
  // 车辆资源占满的快照基线: 初始解上已占满的车型不再触发刷新(此时，候选都是按当前资源评估的)
  const std::vector<VehicleUsage::UPtr>& usages = this->sol->vehicle_resource->vehicle_usages;
  this->vehicle_full_.assign(usages.size(), 0);
  for (std::size_t v = 0; v < usages.size(); ++v) {
    this->vehicle_full_[v] = usages[v]->has_available_vehicle() ? 0 : 1;
  }
}

void RepairMatrix::remove_segment(const int segment_ind) {
  // 保持 idle 的插入顺序 不能用 swap-erase
  std::erase(this->idle_segment_indices, segment_ind);

  RepairRow& row = this->rows[static_cast<std::size_t>(segment_ind)];
  if (row.active) {
    this->feasible_count_ -= static_cast<int>(row.ranked.size());
    row.ranked.clear();
  }
  row.active = false;
  row.cells.clear();
}

void RepairMatrix::commit(const RepairCellView& view) {
  const int segment_ind = view.segment_ind;
  const int load_ind = view.load_ind;
  // check: segment 索引是否有效
  assert(segment_ind >= 0 && segment_ind < static_cast<int>(this->rows.size()));
  // check: load 索引是否有效
  assert(load_ind >= 0 && load_ind < static_cast<int>(this->load_version_.size()));
  RepairRow& row = this->rows[static_cast<std::size_t>(segment_ind)];
  // check: segment必须活跃
  assert(row.active);
  RepairCell& cell = row.cells[static_cast<std::size_t>(load_ind)];
  // check: cell必须可行
  assert(cell.is_feasible);
  // check: 候选必须在评估后未经历该列重算
  assert(cell.version == this->load_version_[static_cast<std::size_t>(load_ind)]);

  const double ori_obj_val = this->sol->loads[static_cast<std::size_t>(load_ind)]->get_obj_val();
  const VehicleResource& vehicle_resource = *this->sol->vehicle_resource;
  // check: 车次索引必须在有效范围内
  assert(cell.vehicle_ind >= 0 && cell.vehicle_ind < vehicle_resource.len);
  const Vehicle* plan_vehicle =
      vehicle_resource.vehicle_usages[static_cast<std::size_t>(cell.vehicle_ind)]->vehicle;
  // 车型资源: 车型不变 或者 新车型有可用资源
  assert(cell.vehicle_ind ==
             static_cast<std::int32_t>(
                 this->sol->loads[static_cast<std::size_t>(load_ind)]->vehicle->ind) ||
         vehicle_resource.has_available(plan_vehicle));

  // 按候选记录的槽位与车型重做插入
  const InsertPlan plan{
      .slot_ind = cell.slot_ind, .vehicle = plan_vehicle, .obj_val = ori_obj_val + cell.delta};
  Load* result_load = this->load_editor->apply_insert(
      this->sol->loads[static_cast<std::size_t>(load_ind)],
      &this->segments[static_cast<std::size_t>(segment_ind)], plan, nullptr);
#ifndef NDEBUG
  // check: 重做后的目标值变化必须等于评估时的 delta
  if (result_load != nullptr) {
    [[maybe_unused]] const double delta_tolerance = 1e-6 * std::max(1.0, std::abs(cell.delta));
    assert(std::abs((result_load->get_obj_val() - ori_obj_val) - cell.delta) <= delta_tolerance);
  }
#endif
  if (result_load == nullptr) {
    spdlog::error(
        "repair commit 失败，使用了过时的 repair 方案: segment={} load={} slot={} vehicle={}",
        segment_ind, load_ind, cell.slot_ind, cell.vehicle_ind);
    throw std::logic_error("RepairMatrix::commit: 使用了过时的 repair 方案");
  }

  if (this->commit_trace != nullptr) {
    this->commit_trace->push_back(view);
  }
  // 替换 solution 中的 load
  this->sol->update_load(load_ind, result_load);
  // 重置 cell
  cell = RepairCell{};
  // 移除 segment 在 solution 中未指派的订单
  this->sol->remove_unassigned_order_by_segment(
      &this->segments[static_cast<std::size_t>(segment_ind)]);
  // 释放 segment_ind 在 matrix 中的信息
  this->remove_segment(segment_ind);
  // 更新剩余 segment 中有关 load_ind 的插入信息
  this->update_load(load_ind);
  // 本次提交可能把某车型占满: 立即重评估仍在用它的格子, 保证下一个候选不携带过时资源
  this->sync_vehicle_fullness();
}

void RepairMatrix::mark_infeasible(const RepairCellView& view) {
  const int segment_ind = view.segment_ind;
  const int load_ind = view.load_ind;
  assert(segment_ind >= 0 && segment_ind < static_cast<int>(this->rows.size()));
  assert(load_ind >= 0 && load_ind < static_cast<int>(this->load_version_.size()));
  RepairRow& row = this->rows[static_cast<std::size_t>(segment_ind)];
  assert(row.active);
  RepairCell& cell = row.cells[static_cast<std::size_t>(load_ind)];
  assert(cell.is_feasible);
  assert(cell.version == this->load_version_[static_cast<std::size_t>(load_ind)]);
  // 下一次列重算若仍可行会重新写回; 此处保证本轮循环单调收敛
  erase_ranked(row.ranked, cell.delta, load_ind);
  cell = RepairCell{};
  --this->feasible_count_;
}

int RepairMatrix::unfold_matrix_cell_count() const {
#ifndef NDEBUG
  // 增量计数与全表重算的一致性断言(长期开启, 仅 Debug 构建有开销)
  int full_count = 0;
  for (const RepairRow& row : this->rows) {
    if (!row.active) {
      continue;
    }
    for (const RepairCell& cell : row.cells) {
      full_count += cell.is_feasible ? 1 : 0;
    }
  }
  assert(full_count == this->feasible_count_);
#endif
  return this->feasible_count_;
}

void RepairMatrix::collect_row_views(const int segment_ind,
                                     std::vector<RepairCellView>& out) const {
  assert(segment_ind >= 0 && segment_ind < static_cast<int>(this->rows.size()));
  const RepairRow& row = this->rows[static_cast<std::size_t>(segment_ind)];
  if (!row.active) {
    return;
  }
  assert(row.cells.size() == this->load_version_.size());
  for (std::size_t l = 0; l < row.cells.size(); ++l) {
    const RepairCell& cell = row.cells[l];
    if (!cell.is_feasible) {
      continue;
    }
    // 版本必须等于列版本, 防止矩阵静默过期
    assert(cell.version == this->load_version_[l]);
    out.push_back(RepairCellView{
        .delta = cell.delta, .segment_ind = segment_ind, .load_ind = static_cast<int>(l)});
  }
}

std::vector<RepairCellView> RepairMatrix::unfold_matrix_top_k(const int k) const {
  std::vector<RepairCellView> candidates;
  for (int s = 0; s < static_cast<int>(this->rows.size()); ++s) {
    this->collect_row_views(s, candidates);
  }
  return RepairRank::select_top_k(std::move(candidates), k);
}

std::vector<RepairCellView> RepairMatrix::unfold_segment_top_k(const int segment_ind,
                                                               const int k) const {
  std::vector<RepairCellView> candidates;
  this->collect_row_views(segment_ind, candidates);
  return RepairRank::select_top_k(std::move(candidates), k);
}

void RepairMatrix::update_load(const int load_ind) {
  if (load_ind < 0) {
    return;
  }
  const auto load_index = static_cast<std::size_t>(load_ind);
  // 新增列: 补齐版本表
  if (this->load_version_.size() <= load_index) {
    this->load_version_.resize(load_index + 1, 0);
  }
  const int idle_segment_count = static_cast<int>(this->idle_segment_indices.size());
  if (idle_segment_count == 0) {
    return;
  }
  // 重算前先推进版本: 之后写入的格都带新版本
  ++this->version_counter_;
  this->load_version_[load_index] = this->version_counter_;
  // 串行补齐新列: 并行阶段不允许任何扩容
  for (int u = 0; u < idle_segment_count; ++u) {
    RepairRow& row = this->rows[static_cast<std::size_t>(this->idle_segment_indices[u])];
    if (row.cells.size() <= load_index) {
      row.cells.resize(load_index + 1);
    }
  }
  this->run_parallel(idle_segment_count, [&](const int u, CacheStoreBuffer* store_buffer) {
    const int segment_ind = this->idle_segment_indices[u];
    RepairRow& row = this->rows[static_cast<std::size_t>(segment_ind)];
    RepairCell& cell = row.cells[load_index];
    // 行内有序视图由本行的唯一写者维护: 先删旧项, 重算后按需插新项
    if (cell.is_feasible) {
      erase_ranked(row.ranked, cell.delta, load_ind);
    }
    cell = this->eval_cell(segment_ind, load_ind, store_buffer);
    if (cell.is_feasible) {
      insert_ranked(row.ranked, cell.delta, load_ind);
    }
  });
  // 串行汇总: 总数不允许在并行阶段被改写
  int feasible_total = 0;
  for (const RepairRow& row : this->rows) {
    if (row.active) {
      feasible_total += static_cast<int>(row.ranked.size());
    }
  }
  this->feasible_count_ = feasible_total;
}

void RepairMatrix::sync_vehicle_fullness() {
  const std::vector<VehicleUsage::UPtr>& usages = this->sol->vehicle_resource->vehicle_usages;
  if (this->vehicle_full_.size() != usages.size()) {
    this->vehicle_full_.assign(usages.size(), 0);
  }
  for (std::size_t v = 0; v < usages.size(); ++v) {
    const bool full = !usages[v]->has_available_vehicle();
    if (full == (this->vehicle_full_[v] != 0)) {
      continue;
    }
    this->vehicle_full_[v] = full ? 1 : 0;
    if (full) {
      // 占满: 仍在用该车型的候选立即过期, 重评估它们
      this->refresh_saturated_vehicle(static_cast<int>(v));
    } else {
      // 空出: 当时把它列为待跟踪的格子可能换到更便宜的车型, 或从不可行变为可行
      this->retry_released_vehicle(static_cast<int>(v));
    }
  }
}

void RepairMatrix::refresh_saturated_vehicle(const int vehicle_ind) {
  const auto saturated = static_cast<std::int32_t>(vehicle_ind);
  for (std::size_t s = 0; s < this->rows.size(); ++s) {
    RepairRow& row = this->rows[s];
    if (!row.active) {
      continue;
    }
    bool dirty = false;
    for (std::size_t l = 0; l < row.cells.size(); ++l) {
      RepairCell& cell = row.cells[l];
      if (!cell.is_feasible || cell.vehicle_ind != saturated) {
        continue;
      }
      // 同车型的候选与空位无关，无需重算
      if (this->sol->loads[l]->vehicle->ind == vehicle_ind) {
        continue;
      }
      cell = this->eval_cell(static_cast<int>(s), static_cast<int>(l), nullptr);
      dirty = true;
    }
    if (dirty) {
      rebuild_ranked(row);
    }
  }
  // 更新 feasible_count_
  int total = 0;
  for (const RepairRow& row : this->rows) {
    if (row.active) {
      total += static_cast<int>(row.ranked.size());
    }
  }
  this->feasible_count_ = total;
}

void RepairMatrix::retry_released_vehicle(const int vehicle_ind) {
  // 只重评估"评估时把该车型列为待跟踪"的格子: 位图由 eval_cell 在选车时写下,
  // 覆盖可行格(可能换成更便宜的车型)与不可行格(可能因此变可行);
  // 位图按 vehicle_ind % 64 分桶, 同桶其他车型释放时命中的格子会偏多
  const std::uint64_t bit = std::uint64_t{1} << (static_cast<unsigned>(vehicle_ind) & 63U);
  for (std::size_t s = 0; s < this->rows.size(); ++s) {
    RepairRow& row = this->rows[s];
    if (!row.active) {
      continue;
    }
    bool dirty = false;
    for (std::size_t l = 0; l < row.cells.size(); ++l) {
      RepairCell& cell = row.cells[l];
      const bool watched = (cell.watched_vehicles & bit) != 0;
      if (!watched) {
        continue;
      }
      cell = this->eval_cell(static_cast<int>(s), static_cast<int>(l), nullptr);
      dirty = true;
    }
    if (dirty) {
      rebuild_ranked(row);
    }
  }
  // 更新 feasible_count_
  int total = 0;
  for (const RepairRow& row : this->rows) {
    if (row.active) {
      total += static_cast<int>(row.ranked.size());
    }
  }
  this->feasible_count_ = total;
}

bool RepairMatrix::idle_segment_create_load() {
  for (const int& segment_ind : this->idle_segment_indices) {
    const Segment& segment = this->segments[static_cast<std::size_t>(segment_ind)];
    // 从空 load 起手: 内部过滤可用车辆并挑选最优车型, 无可行车时返回 infeasible 的 load
    Load* load = this->load_editor->build(segment.orders, *this->sol->vehicle_resource);
    if (load->is_infeasible()) {
      delete load;
      continue;
    }
    // 归档到 solution: 内部占用车辆资源, 并按 orders 擦除未指派订单
    this->sol->add_load(load, segment.orders);
    const int new_load_ind = static_cast<int>(this->sol->loads.size()) - 1;
    // 先移除已消费的 segment, 否则它会参与下面的补列
    this->remove_segment(segment_ind);
    // 为剩余 idle segment 补上新车次这一列, 之后立刻返回: 上面的 erase 已使迭代器失效
    this->update_load(new_load_ind);
    // 新建车次可能占满某车型: 同 commit, 立即刷新仍在用它的格子
    this->sync_vehicle_fullness();
    return true;
  }
  return false;
}
