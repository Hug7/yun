/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#include "algo_parallel_executor.h"

#include <spdlog/spdlog.h>

#include <algorithm>
#include <atomic>
#include <exception>
#include <memory>
#include <thread>
#include <vector>

namespace {

using TaskBody = std::function<void(int, CacheStoreBuffer*)>;

/**
 * @brief 一个并行分片: 以原子计数器动态领取任务下标, 直到领取完
 * @details 分片数与 worker 数相当, 而不是与任务数相当 —— 每个 cell 都 fork
 * 一个协程的开销 (协程帧 + 唤醒)会超过任务本身的收益。同一分片内的多个 cell
 * 共用该分片的写缓冲, 因此跨 cell 的重复键也能命中。
 *
 * 本对象必须保持无状态: fork 的任务对象若带指针成员,
 * 其被销毁后子任务仍可能访问它们,
 * 因此所有入参都通过参数传入(参数会被拷进子任务的帧)。
 * 异常一律在任务内收敛: fork 之后、join 之前抛异常在 libfork 下是未定义行为。
 */
struct ChunkTask {
  auto operator()(auto, const TaskBody* body, std::atomic<int>* cursor, const int task_count,
                  CacheStoreBuffer* store_buffer) const -> lf::task<> {
    while (true) {
      const int u = cursor->fetch_add(1, std::memory_order_relaxed);
      if (u >= task_count) {
        break;
      }
      try {
        (*body)(u, store_buffer);
      } catch (const std::exception& e) {
        spdlog::warn("parallel task {} 失败, 按不可行处理: {}", u, e.what());
      } catch (...) {
        spdlog::warn("parallel task {} 抛出未知异常, 按不可行处理", u);
      }
    }
    co_return;
  }
};

/**
 * @brief 根任务: fork 出 chunk_count-1 个分片, 最后一片由自己 call 执行, 然后
 * join
 * @details 最后一片用 call 而不是 fork, 让提交方线程也参与计算, 不浪费一个核。
 */
struct RootTask {
  auto operator()(auto, const TaskBody* body, std::atomic<int>* cursor, const int task_count,
                  std::vector<std::unique_ptr<CacheStoreBuffer>>& store_buffers,
                  const int chunk_count) const -> lf::task<> {
    for (int c = 0; c < chunk_count - 1; ++c) {
      co_await lf::fork(ChunkTask{})(body, cursor, task_count,
                                     store_buffers[static_cast<std::size_t>(c)].get());
    }
    co_await lf::call(ChunkTask{})(body, cursor, task_count,
                                   store_buffers[static_cast<std::size_t>(chunk_count - 1)].get());
    co_await lf::join;
  }
};

/**
 * @brief 并行阶段守卫: 保证退出时解除冻结表的只读标记, 即使任务体抛出
 */
struct ParallelGuard {
  LoadContext* ctx;

  explicit ParallelGuard(LoadContext* load_context) : ctx(load_context) {
    this->ctx->cache->on_parallel();
  }

  ParallelGuard(const ParallelGuard&) = delete;

  ParallelGuard& operator=(const ParallelGuard&) = delete;

  ~ParallelGuard() { this->ctx->cache->off_parallel(); }
};

}  // namespace

// ====== implement of ParallelExecutor ======
ParallelExecutor::ParallelExecutor(const int worker_count)
    : pool(std::make_unique<lf::lazy_pool>(worker_count > 0
                                               ? static_cast<std::size_t>(worker_count)
                                               : std::thread::hardware_concurrency() - 1)),
      worker_count_(worker_count) {}

ParallelExecutor::~ParallelExecutor() = default;

void ParallelExecutor::run(const int task_count, LoadContext* load_context,
                           const std::function<void(int, CacheStoreBuffer*)>& task) const {
  if (task_count <= 0) {
    return;
  }
  // 分片数取 min(任务数, worker 数 + 提交方): 每片一个写缓冲,
  // 片内以原子计数器动态领取
  const int chunk_count = std::min(task_count, std::max(this->worker_count_, 1) + 1);
  std::vector<std::unique_ptr<CacheStoreBuffer>> store_buffers;
  store_buffers.reserve(chunk_count);
  for (int c = 0; c < chunk_count; ++c) {
    store_buffers.push_back(std::make_unique<CacheStoreBuffer>());
  }

  std::atomic<int> cursor{0};
  {
    const ParallelGuard guard(load_context);
    lf::sync_wait(*this->pool, RootTask{}, &task, &cursor, task_count, store_buffers, chunk_count);
  }
  // 屏障之后单线程归档: 此刻已无任何并发读者
  // 即使上方 sync_wait / archive 抛异常也不会泄漏 (ParallelGuard 仍保证 off_parallel)
  for (auto& store_buffer : store_buffers) {
    if (!store_buffer->empty()) {
      load_context->cache->archive(store_buffer.get());
    }
  }
}
