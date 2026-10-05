/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <atomic>
#include <condition_variable>
#include <cstddef>
#include <exception>
#include <functional>
#include <mutex>
#include <thread>
#include <vector>

/**
 * @brief 线程池
 * @details 只提供 parallel_for: 提交一批同构任务并阻塞等待全部结束。
 * 任务按 [0, task_count) 由各线程动态领取, 提交方线程也参与计算。
 * 并发预算由调用方控制: 需要串行语义时把 worker 数覆盖为 0 即可。
 */
class ThreadPool {
 public:

  explicit ThreadPool(int worker_count);

  ThreadPool(const ThreadPool&) = delete;

  ThreadPool& operator=(const ThreadPool&) = delete;

  ~ThreadPool();

  /**
   * @brief worker 数(不含提交方线程)
   */
  [[nodiscard]] std::size_t worker_count() const { return this->workers.size(); }

  /**
   * @brief 提交一批任务并阻塞等待
   * @param task_count 任务数量, 小于等于 0 时直接返回
   * @param task 任务体, 入参为任务下标
   * @throws 任务抛出的首个异常, 在全部 worker 结束后重抛
   * @details 不可嵌套调用; 任一任务抛异常后设置终止标记, 其余未领取任务被跳过
   */
  void parallel_for(int task_count, const std::function<void(int)>& task);

 private:
  void worker_loop();

  std::vector<std::thread> workers;

  std::mutex mu;

  std::condition_variable cv;

  std::function<void(int)> task;

  /**
   * @brief 本轮任务数量
   */
  int task_count{0};

  /**
   * @brief 下一个待领取的任务下标
   */
  std::atomic<int> next_ind{0};

  /**
   * @brief 批次号, 每轮 parallel_for 自增, worker 据此识别新任务
   */
  std::size_t generation{0};

  /**
   * @brief 本轮已结束的 worker 数
   */
  std::size_t finished_worker_count{0};

  /**
   * @brief 异常终止标记
   */
  std::atomic<bool> stop_flag{false};

  std::exception_ptr first_exception{nullptr};

  bool stopping{false};

  /**
   * @brief 是否有 parallel_for 正在运行, 用于拦截嵌套调用
   */
  std::atomic<bool> running{false};
};
