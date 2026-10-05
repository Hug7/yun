/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "algo_alns_operator.h"
#include "algo_alns_parameter.h"
#include "algo_alns_solution_manager.h"
#include "algo_segment_builder.h"
#include "algo_thread_pool.h"
#include "pdm_solution.h"
#include "pdm_workspace.h"

class AlnsModel {
 public:
  /**
   * @brief 工作台
   */
  const Workspace* workspace;
  /**
   * @brief alns 算法参数
   */
  const AlnsParameter* parameter;
  /**
   * @brief alns 算子管理器
   */
  AlnsOperatorManager* operator_manager;
  /**
   * @brief alns solution管理器
   */
  AlnsSolutionManager* sol_manager;
  /**
   * @brief segment 生成器
   */
  SegmentBuilder* segment_builder;
  /**
   * @brief 线程池
   * @details 由外层调用方持有, 本类不负责释放; 寿命不依赖析构(当前析构不会被执行)
   */
  ThreadPool* thread_pool;

  explicit AlnsModel(const Workspace* _workspace, const AlnsParameter* _parameter,
                     AlnsOperatorManager* operator_manager, ThreadPool* _thread_pool);

  ~AlnsModel();

  [[nodiscard]] Solution* solve() const;
};
