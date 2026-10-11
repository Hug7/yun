/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "pdm_cache_time_window.h"
#include "pdm_parameter.h"

class CacheStoreBuffer {
 public:
  NodeTimeWindowStoreBuffer* node_time_window;

  CacheStoreBuffer();

  ~CacheStoreBuffer();

  [[nodiscard]] bool empty() const { return this->node_time_window->empty(); }
};

/**
 * @brief 缓存
 * @details 存储计算过程中高频使用的数据：时间窗、load等
 */
class Cache {
 public:
  /**
   * @brief 请求内共享的节点时间窗缓存(唯一实例)
   * @details 同一请求的所有工作线程共享这一份: 并行阶段只读, 新增条目先落在任务私有的
   * NodeTimeWindowStoreBuffer 上, 屏障之后由单线程归档进本表。
   */
  NodeTimeWindowCache* node_time_window;

  explicit Cache(const Parameter* parameter);

  ~Cache();

  /**
   * @brief 标记进入并行阶段: 并行阶段冻结表只读, 任何直写都会抛异常
   */
  void on_parallel() const { this->node_time_window->on_parallel(); };

  /**
   * @brief 标记退出并行阶段
   */
  void off_parallel() const { this->node_time_window->off_parallel(); };

  void archive(const CacheStoreBuffer* cache_store_buffer) const;
};
