/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#include "algo_thread_pool.h"

#include <cassert>

// ====== implement of ThreadPool ======
ThreadPool::ThreadPool(const int worker_count) {
  std::lock_guard<std::mutex> guard(this->mu);
  this->workers.reserve(worker_count);
  for (int u = 0; u < worker_count; ++u) {
    this->workers.emplace_back([this] { this->worker_loop(); });
  }
}

ThreadPool::~ThreadPool() {
  {
    std::lock_guard<std::mutex> guard(this->mu);
    this->stopping = true;
  }
  this->cv.notify_all();
  for (auto& worker : this->workers) {
    if (worker.joinable()) {
      worker.join();
    }
  }
}

void ThreadPool::worker_loop() {
  std::unique_lock<std::mutex> lock(this->mu);
  std::size_t last_generation = 0;
  while (true) {
    // generation 变化即新的一批任务; stopping 只有析构会置位
    this->cv.wait(lock, [this, last_generation] {
      return this->generation != last_generation || this->stopping;
    });
    if (this->generation == last_generation) {
      return;
    }
    last_generation = this->generation;
    // 任务体在无锁状态下执行, 只在领取和收尾时同步
    while (!this->stop_flag.load(std::memory_order_relaxed)) {
      const int ind = this->next_ind.fetch_add(1, std::memory_order_relaxed);
      if (ind >= this->task_count) {
        break;
      }
      lock.unlock();
      try {
        this->task(ind);
      } catch (...) {
        std::lock_guard<std::mutex> guard(this->mu);
        if (this->first_exception == nullptr) {
          this->first_exception = std::current_exception();
        }
        this->stop_flag.store(true, std::memory_order_relaxed);
      }
      lock.lock();
    }
    // 终止时把剩余任务索引领完, 保证 next_ind 单调到界
    while (this->stop_flag.load(std::memory_order_relaxed) &&
           this->next_ind.load(std::memory_order_relaxed) < this->task_count) {
      this->next_ind.fetch_add(1, std::memory_order_relaxed);
    }
    ++this->finished_worker_count;
    lock.unlock();
    this->cv.notify_all();
    lock.lock();
  }
}

void ThreadPool::parallel_for(const int task_count, const std::function<void(int)>& task) {
  // 嵌套调用时 worker 的结束计数无法收敛, 会直接死锁
  assert(!this->running.exchange(true, std::memory_order_relaxed));
  if (task_count <= 0) {
    this->running.store(false, std::memory_order_relaxed);
    return;
  }
  std::size_t worker_count = 0;
  {
    std::lock_guard<std::mutex> guard(this->mu);
    this->task = task;
    this->task_count = task_count;
    this->next_ind.store(0, std::memory_order_relaxed);
    this->stop_flag.store(false, std::memory_order_relaxed);
    this->first_exception = nullptr;
    this->finished_worker_count = 0;
    ++this->generation;
    worker_count = this->workers.size();
  }
  this->cv.notify_all();
  // 提交方线程也参与领取, 避免浪费一个核
  while (!this->stop_flag.load(std::memory_order_relaxed)) {
    const int ind = this->next_ind.fetch_add(1, std::memory_order_relaxed);
    if (ind >= task_count) {
      break;
    }
    try {
      task(ind);
    } catch (...) {
      std::lock_guard<std::mutex> guard(this->mu);
      if (this->first_exception == nullptr) {
        this->first_exception = std::current_exception();
      }
      this->stop_flag.store(true, std::memory_order_relaxed);
    }
  }
  // 终止时把剩余任务索引领完, 让 worker 尽快退出本轮
  while (this->stop_flag.load(std::memory_order_relaxed) &&
         this->next_ind.load(std::memory_order_relaxed) < task_count) {
    this->next_ind.fetch_add(1, std::memory_order_relaxed);
  }
  std::exception_ptr exception = nullptr;
  {
    std::unique_lock<std::mutex> lock(this->mu);
    this->cv.wait(lock,
                  [this, worker_count] { return this->finished_worker_count == worker_count; });
    exception = this->first_exception;
  }
  this->running.store(false, std::memory_order_relaxed);
  if (exception != nullptr) {
    std::rethrow_exception(exception);
  }
}
