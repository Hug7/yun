/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <functional>
#include <memory>

#include "algo_libfork.hpp"
#include "pdm_cache.h"
#include "pdm_load.h"

/**
 * @brief 并行执行器: 基于 libfork 的 fork-join 调度器
 * @details 只暴露一个入口 run: 提交一批同构任务并阻塞等待全部结束。
 * 任务体拿到的 CacheStoreBuffer 是任务私有的时间窗写缓冲, run 返回前会把所有写缓冲的新增条目
 * 归档进 LoadContext 持有的 Cache —— 调用方拿不到"不归档"的路径。
 */
class ParallelExecutor {
 public:
  /**
   * @brief 构造并启动 worker
   * @param worker_count worker 数; <=0 时退化为 libfork 的默认(硬件并发数)
   */
  explicit ParallelExecutor(int worker_count);

  ParallelExecutor(const ParallelExecutor&) = delete;

  ParallelExecutor& operator=(const ParallelExecutor&) = delete;

  ~ParallelExecutor();

  [[nodiscard]] int worker_count() const { return this->worker_count_; }

  /**
   * @brief 提交一批任务并阻塞等待全部结束
   * @param task_count 任务数量, <=0 时直接返回
   * @param load_context 请求上下文, 用于标记并行阶段并在屏障后归档
   * @param task 任务体, 入参为任务下标与其私有的写缓冲
   */
  void run(int task_count, LoadContext* load_context,
           const std::function<void(int, CacheStoreBuffer*)>& task) const;

 private:
  std::unique_ptr<lf::lazy_pool> pool;

  int worker_count_{0};
};
