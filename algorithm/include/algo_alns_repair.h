/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <functional>
#include <magic_enum/magic_enum.hpp>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "algo_parallel_executor.h"
#include "algo_segment_builder.h"
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
   * @brief 并行执行器
   * @details 由策略层持有, 本类不负责释放
   */
  ParallelExecutor* parallel_executor;
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
   * @brief matrix[segment_ind][load_ind]
   * @details 内层用按下标索引的 vector: 预建长度后并行阶段只写各自的元素,
   * 不再并发调用同一个容器的非 const 成员函数
   */
  std::unordered_map<int, std::vector<RepairLoad*>> matrix;
  /**
   * @brief 按 segment_ind 直取 matrix 的行指针, 并行阶段只读
   * @details matrix 在预建之后不再插入新键, 故行地址稳定
   */
  std::vector<std::vector<RepairLoad*>*> matrix_rows;

  explicit RepairMatrix(ParallelExecutor* _executor, const SolverContext* _context, Solution* _sol,
                        std::vector<Segment*> _segments);

  ~RepairMatrix();

  void init();

  /**
   * @brief 并行入口: 提交一批任务并阻塞等待
   * @details 任务体拿到的写缓冲是任务私有的时间窗写缓冲; 屏障返回前本函数会把所有写缓冲的新增
   * 条目归档进请求内共享的冻结表。调用方拿不到"不归档"的路径。
   * 任务体抛出的异常在此收敛, 不会穿越并行边界。
   */
  void run_parallel(int task_count, const std::function<void(int, CacheStoreBuffer*)>& task) const;

  /**
   * @brief 计算一格 (segment, load) 的插入结果
   * @details 不可行(含抛异常)时返回 is_feasible=false 的占位对象, 保证 matrix 不留 nullptr
   */
  RepairLoad* eval_cell(int segment_ind, int load_ind, CacheStoreBuffer* store_buffer) const;

  void add_cell(RepairLoad* repair_load);

  void remove_segment(int segment_ind);

  void use_segment_schema(RepairLoad* repair_load);

  int unfold_matrix_cell_count();

  std::vector<RepairLoad*> unfold_matrix_top_k(int k);

  void update_load(int load_ind) const;

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
   * @brief 并行执行器
   * @details 由外层 alns 调用方持有, 本类不负责释放
   */
  ParallelExecutor* parallel_executor;

  virtual ~AlnsRepairOperator() = default;

  explicit AlnsRepairOperator(const AlnsRepairOperatorCode operator_code,
                              const SolverContext* _context, ParallelExecutor* _parallel_executor)
      : code(std::string(magic_enum::enum_name(operator_code))),
        context(_context),
        parallel_executor(_parallel_executor) {}

  virtual void execute(RepairMatrix* repair_matrix) = 0;

  void call(Solution* sol, SegmentBuilder* segment_builder);
};