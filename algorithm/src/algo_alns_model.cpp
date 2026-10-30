/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#include "algo_alns_model.h"

#include "c_chrono_util.h"

// ====== implement of AlnsModel ======
AlnsModel::AlnsModel(const Workspace* _workspace, const AlnsParameter* _parameter,
                     AlnsOperatorManager* operator_manager)
    : workspace(_workspace),
      parameter(_parameter),
      operator_manager(operator_manager),
      sol_manager(new AlnsSolutionManager(workspace, _parameter)),
      segment_builder(new SegmentBuilder()) {}

AlnsModel::~AlnsModel() {
  delete this->sol_manager;
  delete this->segment_builder;
}

Solution* AlnsModel::solve() const {
  const auto logger = this->workspace->context->logger;
  int pre_update_best_sol_iter = 0;                               // 上一次更新最优的迭代次数
  int last_iter = 0;                                              // 最后一次迭代次数
  const auto start_ts = chrono_util::to_timestamp(chrono_util::now());  // 开始时间戳

  for (int iter = 1; iter <= this->parameter->max_iter; ++iter) {
    last_iter = iter;
    // // 重置 cur_sol
    // this->sol_manager->reset_cur_sol();
    // this->sol_manager->reset_tmp_sol();
    // 转动轮盘 max_sub_iter 次
    auto operator_combinations =
        this->operator_manager->roulette_wheel_operator(this->parameter->max_sub_iter);
    for (int sub_iter = 0; sub_iter < this->parameter->max_sub_iter; ++sub_iter) {
      // 执行 ruin 算子
      this->operator_manager->ruin_operator(operator_combinations[sub_iter])
          ->call(this->sol_manager->tmp_sol);
      // 执行 repair 算子
      this->operator_manager->repair_operator(operator_combinations[sub_iter])
          ->call(this->sol_manager->tmp_sol, this->segment_builder);
      // 更新 solution, 使用 sa 判断 tmp_sol 是否被接受
      auto status_code = this->sol_manager->update_sol();
      // 计算算子的分数
      operator_combinations[sub_iter]->cal_operator_score(status_code);
      // 更新-上一次更新最优的迭代次数
      if (status_code == AlnsSaAcceptStatusCode::DOMINATE_BEST_SOLUTION) {
        pre_update_best_sol_iter = sub_iter;
      }
    }
    // 判断是否超时
    auto cur_ts = chrono_util::to_timestamp(chrono_util::now());  // 当前时间戳
    if (cur_ts - start_ts > this->parameter->max_running_time_sec) {
      logger->info(
          "Early termination, out of maximum running time second={}, current iterations={}",
          this->parameter->max_running_time_sec, iter);
    }
    // TODO 执行 local search
    // 更新 sa 的温度
    this->sol_manager->sa->update_temperature();
    // 更新算子权重
    this->operator_manager->update_operator_weights(operator_combinations);
    // 打印日志
    if (iter % this->parameter->print_log_interval_iter == 0) {
      logger->info(std::format("iter={}, best_obj={}, cur_obj={}, tmp_obj={}", iter,
                               this->sol_manager->best_sol->total_obj_val(),
                               this->sol_manager->cur_sol->total_obj_val(),
                               this->sol_manager->tmp_sol->total_obj_val()));
    }
    if (iter - pre_update_best_sol_iter > this->parameter->max_unimproved_iter) {
      logger->info(
          "Early termination, out of maximum unimproved iterations={}, current iterations={}",
          this->parameter->max_unimproved_iter, iter);
      break;
    }
  }
  // 运行时间
  auto running_time_sec = chrono_util::to_timestamp(chrono_util::now()) - start_ts;
  logger->info("The alns calculation has been completed.");
  logger->info(std::format("execute iterations={}, running time={}s", last_iter, running_time_sec));
  logger->info(std::format(
      "obj={}, cost={}, penalty={}, load count={}", this->sol_manager->best_sol->total_obj_val(),
      this->sol_manager->best_sol->total_cost(), this->sol_manager->best_sol->total_penalty(),
      this->sol_manager->best_sol->loads.size()));

  // 克隆 best solution 作为输出解, 保证 sol_manager 可以正常回收
  return this->workspace->deep_copy_sol(this->sol_manager->best_sol);
}