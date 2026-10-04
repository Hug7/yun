/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#include "algo_alns_repair.h"

#include <queue>
#include <ranges>

#include "c_rang_utils.h"

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
RepairMatrix::RepairMatrix(ThreadPool* _thread_pool, const SolverContext* _context, Solution* _sol,
                           std::vector<Segment*> _segments)
    : thread_pool(_thread_pool), context(_context), sol(_sol), segments(std::move(_segments)) {
  this->idle_segment_indices = RangeUtils::range_vector(static_cast<int>(this->segments.size()));
}

RepairMatrix::~RepairMatrix() {
  // 释放 segments
  for (const auto& segment : this->segments) {
    delete segment;
  }
  // 释放 matrix
  for (const auto& row : this->matrix | std::views::values) {
    for (const auto& repair_load : row | std::views::values) {
      delete repair_load;
    }
  }
}

void RepairMatrix::init() {
  const int load_count = static_cast<int>(sol->loads.size());
  const int segments_count = static_cast<int>(segments.size());

  this->matrix.clear();
  this->matrix.reserve(segments_count);
  // 串行预建全部键: 并行阶段只允许改写 value, 任何插入都是未定义行为
  this->matrix.reserve(segments_count);
  for (int s = 0; s < segments_count; ++s) {
    this->matrix[s].reserve(load_count);
    for (int l = 0; l < load_count; ++l) {
      this->matrix[s].emplace(l, nullptr);
    }
  }

  // 并行阶段: 每个 (segment, load) 一格, 行主序动态领取
  this->thread_pool->parallel_for(segments_count * load_count, [&](const int u) {
    const int s = u / load_count;
    const int l = u % load_count;
    Load* load = this->context->problem->greedy_insert_segment_to_load(sol->loads[l], segments[s],
                                                                       sol->vehicle_resource);
    const auto repair_load = new RepairLoad(s, l, sol->loads[l], load);
    this->add_cell(repair_load);
  });
}

void RepairMatrix::add_cell(RepairLoad* repair_load) {
  auto& repair_loads = this->matrix[repair_load->segment_ind];
  if (repair_loads.contains(repair_load->load_ind)) {
    delete repair_loads[repair_load->load_ind];
  }
  this->matrix[repair_load->segment_ind][repair_load->load_ind] = repair_load;
}

void RepairMatrix::remove_segment(const int segment_ind) {
  // 移除 segment
  std::erase(this->idle_segment_indices, segment_ind);
  for (const auto& repair_load : this->matrix[segment_ind] | std::views::values) {
    delete repair_load;
  }
  this->matrix.erase(segment_ind);
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
  for (const auto& rows : this->matrix | std::views::values) {
    for (const auto& repair_load : rows | std::views::values) {
      if (repair_load->is_feasible) {
        res_count++;
      }
    }
  }
  return res_count;
}

std::vector<RepairLoad*> RepairMatrix::unfold_matrix_top_k(const int k) {
  std::vector<RepairLoad*> repair_loads;
  repair_loads.reserve(k);
  std::priority_queue<RepairLoad*, std::vector<RepairLoad*>, RepairLoadCompare> repair_load_pq(
      RepairLoadCompare{}, std::move(repair_loads));
  int repair_load_pq_len = 0;

  for (const auto& rows : this->matrix | std::views::values) {
    for (const auto& repair_load : rows | std::views::values) {
      if (repair_load->is_feasible) {
        if (repair_load_pq_len < k) {
          repair_load_pq.push(repair_load);
          ++repair_load_pq_len;
        } else {
          if (RepairLoadCompare()(repair_load, repair_load_pq.top())) {
            repair_load_pq.pop();
            repair_load_pq.push(repair_load);
          }
        }
      }
    }
  }

  const int pq_full_size = static_cast<int>(repair_load_pq.size());
  std::vector<RepairLoad*> res_repair_loads;
  res_repair_loads.reserve(pq_full_size);
  for (int u = 0; u < pq_full_size; ++u) {
    res_repair_loads.emplace_back(repair_load_pq.top());
    repair_load_pq.pop();
  }

  return res_repair_loads;
}

void RepairMatrix::update_load(const int load_ind) {
  const int idle_segment_count = static_cast<int>(this->idle_segment_indices.size());
  if (idle_segment_count == 0) {
    return;
  }
  this->thread_pool->parallel_for(idle_segment_count, [&](const int u) {
    const int segment_ind = this->idle_segment_indices[u];
    Load* load = this->context->problem->greedy_insert_segment_to_load(
        sol->loads[load_ind], segments[segment_ind], sol->vehicle_resource);
    const auto repair_load = new RepairLoad(segment_ind, load_ind, sol->loads[load_ind], load);
    this->add_cell(repair_load);
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
  auto repair_matrix = new RepairMatrix(this->thread_pool, this->context, sol, std::move(segments));
  repair_matrix->init();
  // 执行 repair 操作
  this->execute(repair_matrix);
  // 释放 repair_matrix
  delete repair_matrix;
}
