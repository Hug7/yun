/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <atomic>
#include <stdexcept>
#include <unordered_map>
#include <vector>

#include "bdm_location.h"
#include "bdm_time_window.h"
#include "c_constant.h"
#include "pdm_node.h"
#include "pdm_order.h"
#include "pdm_parameter.h"

/**
 * @brief 键哈希值的唯一计算入口, 持有键与非持有视图必须算出同一个值
 */
inline int tw_key_hash_code(ActivityType activity_type, int location_ind,
                            const Order* const* orders, int order_len) {
  int hash_code = static_cast<int>(activity_type) + 1;
  hash_code = hash_code * 31 + location_ind;
  for (int u = 0; u < order_len; u++) {
    hash_code = hash_code * 31 + orders[u]->ind;
  }
  return hash_code;
}

/**
 * @brief 节点时间窗键的非持有视图, 只用于查找
 * @details 持有键 NodeTimeWindowKey 复制了一份 orders 向量, 每次构造都要堆分配; 本视图借用调用方的
 * orders 缓冲, 让查找路径(冻结表命中率约 92%)做到零分配。哈希与相等判定与持有键完全一致。
 */
class NodeTimeWindowKeyView {
 public:
  const ActivityType activity_type;

  const int location_ind;

  /** @brief 借用调用方的 orders 缓冲, 非持有 */
  const Order* const* orders;

  const int order_len;

  const int hash_code;

  NodeTimeWindowKeyView(ActivityType activity_type, const Location* location,
                        const std::vector<const Order*>& orders)
      : activity_type(activity_type),
        location_ind(location->ind),
        orders(orders.data()),
        order_len(static_cast<int>(orders.size())),
        hash_code(tw_key_hash_code(activity_type, location->ind, orders.data(),
                                   static_cast<int>(orders.size()))) {}
};

/**
 * @brief 节点时间窗键, 持有 orders, 用作容器内的存储键
 */
class NodeTimeWindowKey {
 public:
  const ActivityType activity_type;

  const Location* location;

  const std::vector<const Order*> orders;

  int order_len;

  int hash_code;

  NodeTimeWindowKey(ActivityType activity_type, const Location* location);

  NodeTimeWindowKey(ActivityType activity_type, const Location* location,
                    const std::vector<const Order*>& orders);

  bool operator==(const NodeTimeWindowKey& other) const {
    if (this->hash_code != other.hash_code) {
      return false;
    }
    if (this->activity_type != other.activity_type) {
      return false;
    }
    if (this->location->ind != other.location->ind) {
      return false;
    }
    if (this->order_len != other.order_len) {
      return false;
    }
    for (int u = 0; u < this->order_len; u++) {
      if (this->orders[u]->ind != other.orders[u]->ind) {
        return false;
      }
    }
    return true;
  }

  bool operator==(const NodeTimeWindowKeyView& other) const {
    if (this->hash_code != other.hash_code) {
      return false;
    }
    if (this->activity_type != other.activity_type) {
      return false;
    }
    if (this->location->ind != other.location_ind) {
      return false;
    }
    if (this->order_len != other.order_len) {
      return false;
    }
    for (int u = 0; u < this->order_len; u++) {
      if (this->orders[u]->ind != other.orders[u]->ind) {
        return false;
      }
    }
    return true;
  }
};

namespace std {
template <>
struct hash<NodeTimeWindowKey> {
  using is_transparent = void;
  size_t operator()(const NodeTimeWindowKey& v) const { return std::hash<int>{}(v.hash_code); }
  size_t operator()(const NodeTimeWindowKeyView& v) const { return std::hash<int>{}(v.hash_code); }
};

template <>
struct equal_to<NodeTimeWindowKey> {
  using is_transparent = void;
  bool operator()(const NodeTimeWindowKey& a, const NodeTimeWindowKey& b) const { return a == b; }
  bool operator()(const NodeTimeWindowKey& a, const NodeTimeWindowKeyView& b) const {
    return a == b;
  }
  bool operator()(const NodeTimeWindowKeyView& a, const NodeTimeWindowKey& b) const {
    return b == a;
  }
};
}  // namespace std

/**
 * @brief 任务私有的时间窗写缓冲(store buffer)
 * @details 这不是一级缓存, 语义等同于 CPU 的 store buffer(write-back buffer): 存在的目的是
 * 让共享冻结表在并行阶段保持只读, 而不是加速查找。并行阶段的所有重算结果都落到这里,
 * 屏障之后由 archive() 一次性写回冻结表, 方向单向、不回写。
 * 查找顺序是「先冻结表(命中率约 99.6%)后本写缓冲」, 因为本缓冲只可能装冻结表没有的新键;
 * 实测同一并行阶段内本缓冲的命中率只有约 0.6%:
 */
class NodeTimeWindowStoreBuffer {
 public:
  /**
   * @brief 构造查找 key 时复用的订单缓冲, 容量稳定后每次查找零分配
   */
  std::vector<const Order*> orders_scratch;

  std::unordered_map<NodeTimeWindowKey, std::vector<TimeWindow*>> pick;

  std::unordered_map<NodeTimeWindowKey, std::vector<TimeWindow*>> drop;

  std::unordered_map<NodeTimeWindowKey, std::vector<TimeWindow*>> other;

  ~NodeTimeWindowStoreBuffer();

  [[nodiscard]] bool empty() const {
    return this->pick.empty() && this->drop.empty() && this->other.empty();
  }

  /**
   * @brief 查找 pick 并写入本写缓冲
   * @param node pick node
   * @param node_tw_key node 时间窗key
   * @return 返回新条目的时间窗
   */
  const std::vector<TimeWindow*>* get_or_create_pick(const Node* node,
                                                     const NodeTimeWindowKey& node_tw_key);

  /**
   * @brief 查找 drop 并写入本写缓冲
   * @param node drop node
   * @param node_tw_key node 时间窗key
   * @return 返回新条目的时间窗
   */
  const std::vector<TimeWindow*>* get_or_create_drop(const Node* node,
                                                     const NodeTimeWindowKey& node_tw_key);

  /**
   * @brief 计算并写入本写缓冲, 返回新条目的时间窗
   */
  const std::vector<TimeWindow*>* get_or_create_other(const NodeTimeWindowKey& node_tw_key,
                                                      const PlanDatetimeRange* plan_datetime_range);
};

/**
 * @brief 请求内共享的节点时间窗缓存(冻结表)
 * @details 一个 LoadContext 持有唯一实例, 同一请求的所有工作线程共享, 因此天然做到了
 * 跨线程去重。并行阶段对本表只读: 任何写入路径都必须走 NodeTimeWindowStoreBuffer 并在屏障之后
 * 由单线程归档。串行阶段(无写缓冲)允许直写, 由 parallel 标记做硬性校验。
 */
class NodeTimeWindowCache {
 public:
  const PlanDatetimeRange* plan_datetime_range;

  std::unordered_map<NodeTimeWindowKey, std::vector<TimeWindow*>> pick;

  std::unordered_map<NodeTimeWindowKey, std::vector<TimeWindow*>> drop;

  std::unordered_map<NodeTimeWindowKey, std::vector<TimeWindow*>> other;

  explicit NodeTimeWindowCache(const PlanDatetimeRange* plan_datetime_range);

  ~NodeTimeWindowCache();

  /**
   * @brief 只读查找 pick 条目, 未命中返回 nullptr, 绝不写入本表
   * @param scratch 调用方提供的 orders 复用缓冲, 必须来自当前任务私有对象
   */
  [[nodiscard]] const std::vector<TimeWindow*>* find_pick(const Node* node,
                                                          std::vector<const Order*>& scratch) const;

  /**
   * @brief 只读查找 drop 条目, 未命中返回 nullptr, 绝不写入本表
   * @param scratch 调用方提供的 orders 复用缓冲, 必须来自当前任务私有对象
   */
  [[nodiscard]] const std::vector<TimeWindow*>* find_drop(const Node* node,
                                                          std::vector<const Order*>& scratch) const;

  /**
   * @brief 只读查找 other 条目, 未命中返回 nullptr
   */
  [[nodiscard]] const std::vector<TimeWindow*>* find_other(const Node* node) const;

  /**
   * @brief 串行阶段直写: 未命中则计算并入库
   * @param node: 提货节点
   * @param node_time_window_key: 节点的时间窗key
   * @throws std::logic_error 并行阶段调用
   */
  const std::vector<TimeWindow*>* get_or_create_pick(const Node* node,
                                                     const NodeTimeWindowKey& node_time_window_key);

  /**
   * @brief 串行阶段直写: 未命中则计算并入库
   * @param node: 卸货节点
   * @param node_time_window_key: 节点的时间窗key
   * @throws std::logic_error 并行阶段调用
   */
  const std::vector<TimeWindow*>* get_or_create_drop(const Node* node,
                                                     const NodeTimeWindowKey& node_time_window_key);

  /**
   * @brief 串行阶段直写: 未命中则计算并入库
   * @throws std::logic_error 并行阶段调用
   */
  const std::vector<TimeWindow*>* get_or_create_other(const NodeTimeWindowKey& node_tw_key);

  /**
   * @brief 把写缓冲的新增条目归档进本表(必须在屏障之后的串行阶段调用)
   * @details 重复键的 TimeWindow* 会被显式 delete:写缓冲独占自己 new 出来的时间窗,
   * 归档是所有权转移, 未被接收的那一份必须释放。
   */
  void archive(NodeTimeWindowStoreBuffer* node_tw_store_buffer);

  /**
   * @brief 统一的时间窗查找入口
   * @details 依次查本表(只读, 命中率约 99.6%) →写缓冲(任务私有) → 计算并写入写缓冲。
   * store_buffer 为 nullptr 时表示串行阶段, 直接读写本表。
   * 返回的引用在本次推导内有效, 不得跨越归档点长期持有。
   */
  const std::vector<TimeWindow*>& lookup(NodeTimeWindowStoreBuffer* node_tw_store_buffer,
                                         const Node* node);

  /**
   * @brief 标记进入并行阶段, 并行阶段禁止对本表写入
   */
  void on_parallel() { this->parallel.store(true, std::memory_order_relaxed); }

  /**
   * @brief 标记退出并行阶段
   */
  void off_parallel() { this->parallel.store(false, std::memory_order_relaxed); }

  [[nodiscard]] bool is_parallel() const { return this->parallel.load(std::memory_order_relaxed); }

 private:
  /**
   * @brief 串行路径专用的 orders 复用缓冲, 并行阶段不得触碰
   */
  std::vector<const Order*> serial_scratch;

  std::atomic<bool> parallel{false};
};
