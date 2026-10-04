/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <cassert>
#include <magic_enum/magic_enum.hpp>
#include <string>
#include <unordered_map>
#include <unordered_set>

#include "algo_segment_builder.h"
#include "algo_thread_pool.h"
#include "pdm_context.h"
#include "pdm_segment.h"
#include "pdm_solution.h"

/**
 * @brief alns repair算子编码
 */
enum class AlnsRepairOperatorCode {
  RepairGreedy,
};

constexpr AlnsRepairOperatorCode parse_alns_repair_operator_code(const std::string& code) {
  if (code == "RepairGreedy") {
    return AlnsRepairOperatorCode::RepairGreedy;
  } else {
    throw std::invalid_argument("Invalid AlnsRepairOperatorCode = " + code);
  }
}

class RepairLoad {
 public:
  const int segment_ind;

  const int load_ind;

  const Load* raw_load;

  Load* load;

  bool is_feasible;

  double delta;

  RepairLoad(int _segment_ind, int _load_ind, const Load* _raw_load, Load* _load);

  ~RepairLoad();

  Load* move_load();
};

struct RepairLoadCompare {
  bool operator()(const RepairLoad* a, const RepairLoad* b) const {
    if (a == nullptr || b == nullptr) return false;
    if (a->delta != b->delta) {
      return a->delta < b->delta;
    }
    if (a->segment_ind != b->segment_ind) {
      return a->segment_ind < b->segment_ind;
    }
    return a->load_ind < b->load_ind;
  }
};

class RepairMatrix {
 public:
  /**
   * @brief 线程池
   * @details 由外层 alns 调用方持有, 本类不负责释放
   */
  ThreadPool* thread_pool;
  /**
   * @brief solver上下文
   */
  const SolverContext* context;
  /**
   * @brief solution
   */
  Solution* sol;
  /**
   * @brief segment 列表
   */
  const std::vector<Segment*> segments;
  /**
   * @brief 待分配的 segment 索引列表
   */
  std::vector<int> idle_segment_indices;
  /**
   * @brief
   */
  std::unordered_map<int, std::unordered_map<int, RepairLoad*>> matrix;

  explicit RepairMatrix(ThreadPool* _thread_pool, const SolverContext* _context, Solution* _sol,
                        std::vector<Segment*> _segments);

  ~RepairMatrix();

  void init();

  void add_cell(RepairLoad* repair_load);

  void remove_segment(int segment_ind);

  void use_segment_schema(RepairLoad* repair_load);

  int unfold_matrix_cell_count();

  std::vector<RepairLoad*> unfold_matrix_top_k(int k);

  void update_load(int load_ind);

  bool idle_segment_create_load();
};

/**
 * @brief alns repair算子父类
 */
class AlnsRepairOperator {
 public:
  /**
   * @brief repair算子编码
   */
  const std::string code;
  /**
   * @brief solver上下文
   */
  const SolverContext* context;
  /**
   * @brief 线程池
   * @details 由外层 alns 调用方持有, 本类不负责释放
   */
  ThreadPool* thread_pool;

  virtual ~AlnsRepairOperator() = default;

  explicit AlnsRepairOperator(const AlnsRepairOperatorCode operator_code,
                              const SolverContext* _context, ThreadPool* _thread_pool)
      : code(std::string(magic_enum::enum_name(operator_code))),
        context(_context),
        thread_pool(_thread_pool) {}

  virtual void execute(RepairMatrix* repair_matrix) = 0;

  void call(Solution* sol, SegmentBuilder* segment_builder);
};