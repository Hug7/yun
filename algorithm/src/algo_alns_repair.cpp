/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#include "algo_alns_repair.h"

#include <spdlog/spdlog.h>

#include <queue>
#include <ranges>
#include <vector>

#include "c_rang_utils.h"

namespace {
using RepairLoadQueue =
    std::priority_queue<RepairLoad*, std::vector<RepairLoad*>, RepairLoadCompare>;

/**
 * @brief 维护 top-k 可行元素的最大堆
 * @param queue_len 堆中元素数量
 * @param k 保留的候选数量
 */
void try_push_top_k(RepairLoadQueue& queue, int& queue_len, const int k, RepairLoad* repair_load) {
  if (k <= 0 || repair_load == nullptr || !repair_load->is_feasible) {
    return;
  }
  if (queue_len < k) {
    queue.push(repair_load);
    ++queue_len;
  } else if (RepairLoadCompare()(repair_load, queue.top())) {
    queue.pop();
    queue.push(repair_load);
  }
}

/**
 * @brief 弹出堆中全部元素, 得到按 delta 降序排列的结果
 */
std::vector<RepairLoad*> drain_top_k(RepairLoadQueue& queue) {
  std::vector<RepairLoad*> repair_loads;
  repair_loads.reserve(queue.size());
  while (!queue.empty()) {
    repair_loads.emplace_back(queue.top());
    queue.pop();
  }
  return repair_loads;
}
}  // namespace

// ====== implement of RepairLoad ======
RepairLoad::RepairLoad(const int _segment_ind, const int _load_ind, const Load* _raw_load,
                       Load* _load)
    : segment_ind{_segment_ind}, load_ind{_load_ind}, raw_load{_raw_load}, load{_load} {
  if (this->load == nullptr || this->load->is_infeasible()) {
    this->is_feasible = false;
    this->delta = 0;
  } else {
    this->is_feasible = true;
    this->delta = this->load->get_obj_val() - this->raw_load->get_obj_val();
  }
}

RepairLoad::~RepairLoad() { delete this->load; }

Load* RepairLoad::move_load() {
  const auto rws_load = this->load;
  this->load = nullptr;
  return rws_load;
}

// ====== implement of RepairMatrix ======
RepairMatrix::RepairMatrix(ParallelExecutor* _executor, const SolverContext* _context,
                           Solution* _sol, std::vector<Segment*> _segments)
    : parallel_executor(_executor), context(_context), sol(_sol), segments(std::move(_segments)) {
  this->idle_segment_indices = RangeUtils::range_vector(static_cast<int>(this->segments.size()));
}

RepairMatrix::~RepairMatrix() {
  // 释放 segments
  for (const auto& segment : this->segments) {
    delete segment;
  }
  // 释放 matrix
  for (const auto& row : this->matrix | std::views::values) {
    for (const auto& repair_load : row) {
      delete repair_load;
    }
  }
}

void RepairMatrix::run_parallel(const int task_count,
                                const std::function<void(int, CacheStoreBuffer*)>& task) const {
  if (task_count <= 0) {
    return;
  }
  LoadContext* load_context = this->context->problem->load_context;
  // 归档内置在并行入口中: run 返回前所有写缓冲都已并入冻结表
  this->parallel_executor->run(
      task_count, load_context, [&](const int u, CacheStoreBuffer* store_buffer) {
        try {
          task(u, store_buffer);
        } catch (const std::exception& e) {
          // 异常一律收敛在任务体内, 不穿越并行边界: 单个 cell 算不出来不应中断整个 repair
          spdlog::warn("repair task {} 失败, 按不可行处理: {}", u, e.what());
        } catch (...) {
          spdlog::warn("repair task {} 抛出未知异常, 按不可行处理", u);
        }
      });
}

RepairLoad* RepairMatrix::eval_cell(const int segment_ind, const int load_ind,
                                    CacheStoreBuffer* store_buffer) const {
  Load* load = nullptr;
  try {
    load = this->context->problem->greedy_insert_segment_to_load(
        this->sol->loads[load_ind], this->segments[segment_ind], this->sol->vehicle_resource,
        store_buffer);
  } catch (const std::exception& e) {
    // 该 cell 不可行: 收敛成占位对象, 不让异常冒穿并行边界
    spdlog::warn("repair cell (segment={}, load={}) 插入失败: {}", segment_ind, load_ind, e.what());
    load = nullptr;
  }
  if (load != nullptr) {
    // 写缓冲的生命周期只到任务结束, load 可能长期存活: 先解绑, 避免留下悬垂指针
    load->store_buffer = nullptr;
  }
  return new RepairLoad(segment_ind, load_ind, this->sol->loads[load_ind], load);
}

void RepairMatrix::init() {
  const int load_count = static_cast<int>(sol->loads.size());
  const int segments_count = static_cast<int>(segments.size());

  this->matrix.clear();
  this->matrix.reserve(segments_count);
  // 串行预建全部行: 并行阶段只允许改写各自的元素, 任何插入/扩容都是未定义行为
  this->matrix_rows.assign(segments_count, nullptr);
  for (int s = 0; s < segments_count; ++s) {
    auto& row = this->matrix[s];
    row.assign(load_count, nullptr);
    this->matrix_rows[s] = &row;
  }

  // 并行阶段: 每个 (segment, load) 一格, 行主序动态领取
  this->run_parallel(segments_count * load_count, [&](const int u, CacheStoreBuffer* store_buffer) {
    const int s = u / load_count;
    const int l = u % load_count;
    std::vector<RepairLoad*>& row = *this->matrix_rows[s];
    delete row[l];
    row[l] = this->eval_cell(s, l, store_buffer);
  });
}

void RepairMatrix::add_cell(RepairLoad* repair_load) {
  std::vector<RepairLoad*>& row =
      *this->matrix_rows[static_cast<std::size_t>(repair_load->segment_ind)];
  const auto ind = static_cast<std::size_t>(repair_load->load_ind);
  delete row[ind];
  row[ind] = repair_load;
}

void RepairMatrix::remove_segment(const int segment_ind) {
  // 移除 segment
  std::erase(this->idle_segment_indices, segment_ind);
  for (const auto& repair_load : this->matrix[segment_ind]) {
    delete repair_load;
  }
  this->matrix.erase(segment_ind);
  this->matrix_rows[static_cast<std::size_t>(segment_ind)] = nullptr;
}

void RepairMatrix::use_segment_schema(RepairLoad* repair_load) {
  const int segment_ind = repair_load->segment_ind;
  const int load_ind = repair_load->load_ind;
  // 换出要输出的 load
  const auto res_load = repair_load->move_load();
  // 更新 solution 中的 load
  this->sol->update_load(repair_load->load_ind, res_load);
  // 移除 segment 在 solution 中未指派的订单
  this->sol->remove_unassigned_order_by_segment(this->segments[segment_ind]);
  // 释放 segment_ind 在 matrix 中的信息
  this->remove_segment(segment_ind);
  // 更新剩余 segment 中有关 load_ind 的插入信息
  this->update_load(load_ind);
}

int RepairMatrix::unfold_matrix_cell_count() {
  int res_count = 0;
  for (const auto& row : this->matrix | std::views::values) {
    for (const auto& repair_load : row) {
      if (repair_load != nullptr && repair_load->is_feasible) {
        res_count++;
      }
    }
  }
  return res_count;
}

std::vector<RepairLoad*> RepairMatrix::unfold_matrix_top_k(const int k) {
  RepairLoadQueue repair_load_pq;
  int repair_load_pq_len = 0;

  for (const auto& row : this->matrix | std::views::values) {
    for (const auto& repair_load : row) {
      try_push_top_k(repair_load_pq, repair_load_pq_len, k, repair_load);
    }
  }

  return drain_top_k(repair_load_pq);
}

std::vector<RepairLoad*> RepairMatrix::unfold_segment_top_k(const int segment_ind,
                                                            const int k) const {
  RepairLoadQueue repair_load_pq;
  int repair_load_pq_len = 0;

  if (segment_ind >= 0 && segment_ind < static_cast<int>(this->matrix_rows.size())) {
    const std::vector<RepairLoad*>* row = this->matrix_rows[segment_ind];
    if (row != nullptr) {
      for (const auto& repair_load : *row) {
        try_push_top_k(repair_load_pq, repair_load_pq_len, k, repair_load);
      }
    }
  }

  return drain_top_k(repair_load_pq);
}

void RepairMatrix::update_load(const int load_ind) const {
  const int idle_segment_count = static_cast<int>(this->idle_segment_indices.size());
  if (idle_segment_count == 0) {
    return;
  }
  // 串行补齐新列: 并行阶段不允许任何扩容
  for (int u = 0; u < idle_segment_count; ++u) {
    std::vector<RepairLoad*>* row =
        this->matrix_rows[static_cast<std::size_t>(this->idle_segment_indices[u])];
    if (static_cast<int>(row->size()) <= load_ind) {
      row->resize(static_cast<std::size_t>(load_ind) + 1, nullptr);
    }
  }
  this->run_parallel(idle_segment_count, [&](const int u, CacheStoreBuffer* store_buffer) {
    const int segment_ind = this->idle_segment_indices[u];
    std::vector<RepairLoad*>& row = *this->matrix_rows[static_cast<std::size_t>(segment_ind)];
    delete row[static_cast<std::size_t>(load_ind)];
    row[static_cast<std::size_t>(load_ind)] = this->eval_cell(segment_ind, load_ind, store_buffer);
  });
}

bool RepairMatrix::idle_segment_create_load() {
  for (const int& segment_ind : this->idle_segment_indices) {
    auto segment = this->segments[segment_ind];
    // 从空 load 起手: 内部过滤可用车辆并挑选最优车型, 无可行车时返回 infeasible 的 load
    Load* load = this->context->problem->construct_load_by_order(segment->orders,
                                                                 this->sol->vehicle_resource);
    if (load->is_infeasible()) {
      delete load;
      continue;
    }
    // 归档到 solution: 内部占用车辆资源, 并按 orders 擦除未指派订单
    this->sol->add_load(load, segment->orders);
    const int new_load_ind = static_cast<int>(this->sol->loads.size()) - 1;
    // 先移除已消费的 segment, 否则它会参与下面的补列
    this->remove_segment(segment_ind);
    // 为剩余 idle segment 补上新车次这一列, 之后立刻返回: 上面的 erase 已使迭代器失效
    this->update_load(new_load_ind);
    return true;
  }
  return false;
}

// ====== implement of AlnsRepairOperator ======
void AlnsRepairOperator::call(Solution* sol, SegmentBuilder* segment_builder) {
  // 生成 segments
  auto segments = segment_builder->call(sol);
  // 构造 load 和 segment 的成本矩阵
  const auto repair_matrix = std::make_unique<RepairMatrix>(this->parallel_executor, this->context,
                                                            sol, std::move(segments));
  repair_matrix->init();
  // 执行 repair 操作
  this->execute(repair_matrix.get());
}
