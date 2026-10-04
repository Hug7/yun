/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#include "algo_alns_repair_greedy.h"

#include "c_numerical_utils.h"

// ====== implement of AlnsRepairGreedy ======
AlnsRepairGreedy::AlnsRepairGreedy(const SolverContext* _context, ThreadPool* _thread_pool,
                                   const sol::table& cfg)
    : AlnsRepairOperator(AlnsRepairOperatorCode::RepairGreedy, _context, _thread_pool) {
  if (cfg[AlnsRepairGreedyLuaField::RANDOM_SEED].valid()) {
    this->random_seed = static_cast<int>(cfg[AlnsRepairGreedyLuaField::RANDOM_SEED]);
  }
  this->random = std::make_unique<RandomUtils>(this->random_seed);

  if (cfg[AlnsRepairGreedyLuaField::TOP_RATE].valid()) {
    this->top_rate = static_cast<double>(cfg[AlnsRepairGreedyLuaField::TOP_RATE]);
    if (this->top_rate < 0 || this->top_rate > 1.0) {
      throw std::invalid_argument(this->code + ": `top_rate` must be between 0 and 1.");
    }
  }
};

void AlnsRepairGreedy::execute(RepairMatrix* repair_matrix) {
  const size_t raw_idle_segment_count = repair_matrix->idle_segment_indices.size();
  for (size_t r = 0; r < raw_idle_segment_count; ++r) {
    // 获取 repair matrix 有效元素的数量
    const int matrix_cell_count = repair_matrix->unfold_matrix_cell_count();
    if (matrix_cell_count > 0) {
      // 获取 top-k 的值
      const int top_k = std::max(NumUtil::double_to_int(matrix_cell_count * this->top_rate), 1);
      // 获取 repair matrix 的 top-k 元素
      auto repair_loads = repair_matrix->unfold_matrix_top_k(top_k);
      // 在 top-k 元素中选取一个
      const int repair_load_len = static_cast<int>(repair_loads.size());
      const int select_ind = this->random->random_int(1, repair_load_len) - 1;
      const auto select_repair_load = repair_loads[select_ind];
      repair_loads.clear();
      // 落实选中的 repair_load
      repair_matrix->use_segment_schema(select_repair_load);
    } else {
      // 将未指派的 segment 生成新的车次
      const bool suc_flag = repair_matrix->idle_segment_create_load();
      if (!suc_flag) {
        break;
      }
    }
  }
}
