/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#include "algo_alns_model.h"

// ====== implement of AlnsModel ======
AlnsModel::AlnsModel(const Workspace* _workspace, const AlnsParameter* _parameter,
                     AlnsOperatorManager* operator_manager, ThreadPool* _thread_pool)
    : workspace(_workspace),
      parameter(_parameter),
      operator_manager(operator_manager),
      sol_manager(new AlnsSolutionManager(workspace, _parameter)),
      segment_builder(new SegmentBuilder()),
      thread_pool(_thread_pool) {}

AlnsModel::~AlnsModel() {
  delete this->parameter;
  delete this->operator_manager;
  delete this->sol_manager;
  delete this->segment_builder;
}

Solution* AlnsModel::solve() {
  auto logger = this->workspace->context->logger;
  int pre_update_best_sol_iter = -1;

  for (int iter = 0; iter < this->parameter->max_iter; ++iter) {
    // 重置 cur_sol
    this->sol_manager->reset_cur_sol();
    Solution* tmp_sol = this->workspace->deep_copy_sol(this->sol_manager->cur_sol);
    // 转动轮盘 max_sub_iter 次
    auto operator_combinations =
        this->operator_manager->roulette_wheel_operator(this->parameter->max_sub_iter);
    for (int sub_iter = 0; sub_iter < this->parameter->max_sub_iter; ++sub_iter) {
      // 执行 ruin 算子
      this->operator_manager->ruin_operator(operator_combinations[sub_iter])->call(tmp_sol);
      // 执行 repair 算子
      this->operator_manager->repair_operator(operator_combinations[sub_iter])->call(tmp_sol, this->segment_builder);
      // TODO 执行 local search
      // 更新 solution, 使用 sa 判断 tmp_sol 是否被接受
      auto status_code = this->sol_manager->update_sol(tmp_sol);
      // 计算算子的分数
      operator_combinations[sub_iter]->cal_operator_score(status_code);
    }
    // 释放 tmp_sol
    delete tmp_sol;
    // 更新算子权重
    this->operator_manager->update_operator_weights(operator_combinations);
    // 打印日志
    logger->info(std::format("iter={}", iter));
  }

  // 克隆 best solution 作为输出解, 保证 sol_manager 可以正常回收
  return this->workspace->deep_copy_sol(this->sol_manager->best_sol);
}