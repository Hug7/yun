/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <algorithm>
#include <atomic>
#include <cstdint>
#include <stdexcept>
#include <unordered_map>
#include <vector>

#include "bdm_time_window.h"
#include "pdm_node.h"
#include "pdm_order.h"
#include "pdm_parameter.h"

/**
 * @brief 把订单时间窗模式 id 列表整理成缓存键的规范形式: 升序去重
 * @details 缓存值只取决于节点上各订单时间窗内容的集合, 与订单顺序和重复无关, 因此键必须先
 * 规范化; 持有键与非持有视图都只在规范形式上比较。
 */
inline void tw_key_canonicalize(std::vector<int>& pattern_ids) {
  std::ranges::sort(pattern_ids);
  pattern_ids.erase(std::ranges::unique(pattern_ids).begin(), pattern_ids.end());
}

/**
 * @brief 键哈希值的唯一计算入口, 持有键与非持有视图必须算出同一个值
 */
inline std::uint64_t tw_key_hash_code(const int* pattern_ids, int pattern_len) {
  std::uint64_t hash_code = 1;
  for (int u = 0; u < pattern_len; u++) {
    hash_code = hash_code * 31 + pattern_ids[u];
  }
  return hash_code;
}

/**
 * @brief 节点时间窗键的持有形式, 定义见后
 */
class NodeTimeWindowKey;

/**
 * @brief 节点时间窗键的非持有视图, 只用于查找
 * @details 持有键 NodeTimeWindowKey 复制了一份 pattern id 向量, 每次构造都要堆分配; 本视图
 * 借用调用方的缓冲, 让查找路径做到零分配。哈希与相等判定与持有键完全一致,
 * 传入的列表必须已经过 tw_key_canonicalize 规范化。
 */
class NodeTimeWindowKeyView {
 public:
  /** @brief 借用调用方的 pattern id 缓冲, 非持有, 必须升序去重 */
  const int* pattern_ids;

  const int pattern_len;

  const std::uint64_t hash_code;

  explicit NodeTimeWindowKeyView(const std::vector<int>& pattern_ids)
      : pattern_ids(pattern_ids.data()),
        pattern_len(static_cast<int>(pattern_ids.size())),
        hash_code(tw_key_hash_code(pattern_ids.data(), static_cast<int>(pattern_ids.size()))) {}

  bool operator==(const NodeTimeWindowKey& other) const;
};

/**
 * @brief 节点时间窗键, 持有 pattern id 列表, 用作容器内的存储键
 * @details 键只由节点上订单的时间窗模式 id 集合决定: 同一批时间窗内容无论来自哪些订单、
 * 以什么顺序排列, 都落在同一条缓存上; 订单身份变化不再产生新键。
 */
class NodeTimeWindowKey {
 public:
  /**
   * @brief 已规范化的 pattern id 列表(升序去重)
   */
  const std::vector<int> pattern_ids;

  const int pattern_len;

  const std::uint64_t hash_code;

  /**
   * @brief 构造空键, 用于 other 节点(其值恒定, 整表至多一条)
   */
  explicit NodeTimeWindowKey(std::vector<int> pattern_ids = {});

  bool operator==(const NodeTimeWindowKey& other) const {
    if (this->hash_code != other.hash_code || this->pattern_len != other.pattern_len) {
      return false;
    }
    for (int u = 0; u < this->pattern_len; u++) {
      if (this->pattern_ids[u] != other.pattern_ids[u]) {
        return false;
      }
    }
    return true;
  }

  bool operator==(const NodeTimeWindowKeyView& other) const {
    if (this->hash_code != other.hash_code || this->pattern_len != other.pattern_len) {
      return false;
    }
    for (int u = 0; u < this->pattern_len; u++) {
      if (this->pattern_ids[u] != other.pattern_ids[u]) {
        return false;
      }
    }
    return true;
  }
};

inline bool NodeTimeWindowKeyView::operator==(const NodeTimeWindowKey& other) const {
  return other == *this;
}

namespace std {
template <>
struct hash<NodeTimeWindowKey> {
  using is_transparent = void;
  size_t operator()(const NodeTimeWindowKey& v) const {
    return std::hash<std::uint64_t>{}(v.hash_code);
  }
  size_t operator()(const NodeTimeWindowKeyView& v) const {
    return std::hash<std::uint64_t>{}(v.hash_code);
  }
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
   * @brief 构造查找 key 时复用的 pattern id 缓冲, 容量稳定后每次查找零分配
   */
  std::vector<int> patterns_scratch;

  std::unordered_map<NodeTimeWindowKey, std::vector<TimeWindow>> pick;

  std::unordered_map<NodeTimeWindowKey, std::vector<TimeWindow>> drop;

  std::unordered_map<NodeTimeWindowKey, std::vector<TimeWindow>> other;

  [[nodiscard]] bool empty() const {
    return this->pick.empty() && this->drop.empty() && this->other.empty();
  }

  /**
   * @brief 查找 pick 并写入本写缓冲
   * @param node pick node
   * @param node_tw_key node 时间窗key
   * @return 返回新条目的时间窗
   */
  const std::vector<TimeWindow>* get_or_create_pick(const Node* node,
                                                    const NodeTimeWindowKey& node_tw_key);

  /**
   * @brief 查找 drop 并写入本写缓冲
   * @param node drop node
   * @param node_tw_key node 时间窗key
   * @return 返回新条目的时间窗
   */
  const std::vector<TimeWindow>* get_or_create_drop(const Node* node,
                                                    const NodeTimeWindowKey& node_tw_key);

  /**
   * @brief 计算并写入本写缓冲, 返回新条目的时间窗
   */
  const std::vector<TimeWindow>* get_or_create_other(const NodeTimeWindowKey& node_tw_key,
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

  std::unordered_map<NodeTimeWindowKey, std::vector<TimeWindow>> pick;

  std::unordered_map<NodeTimeWindowKey, std::vector<TimeWindow>> drop;

  std::unordered_map<NodeTimeWindowKey, std::vector<TimeWindow>> other;

  explicit NodeTimeWindowCache(const PlanDatetimeRange* plan_datetime_range);

  /**
   * @brief 只读查找 pick 条目, 未命中返回 nullptr, 绝不写入本表
   * @param node 提货节点
   * @param patterns_scratch 调用方提供的 pattern id 复用缓冲
   */
  [[nodiscard]] const std::vector<TimeWindow>* find_pick(const Node* node,
                                                         std::vector<int>& patterns_scratch) const;

  /**
   * @brief 只读查找 drop 条目, 未命中返回 nullptr, 绝不写入本表
   * @param node 卸货节点
   * @param patterns_scratch 调用方提供的 pattern id 复用缓冲
   */
  [[nodiscard]] const std::vector<TimeWindow>* find_drop(const Node* node,
                                                         std::vector<int>& patterns_scratch) const;

  /**
   * @brief 只读查找 other 条目, 未命中返回 nullptr
   * @details other 节点的值恒定, 整表至多一条, 因此不需要节点参数
   */
  [[nodiscard]] const std::vector<TimeWindow>* find_other() const;

  /**
   * @brief 串行阶段直写: 未命中则计算并入库
   * @param node 提货节点
   * @param node_time_window_key 节点的时间窗key
   * @throws std::logic_error 并行阶段调用
   */
  const std::vector<TimeWindow>* get_or_create_pick(const Node* node,
                                                    const NodeTimeWindowKey& node_time_window_key);

  /**
   * @brief 串行阶段直写: 未命中则计算并入库
   * @param node: 卸货节点
   * @param node_time_window_key: 节点的时间窗key
   * @throws std::logic_error 并行阶段调用
   */
  const std::vector<TimeWindow>* get_or_create_drop(const Node* node,
                                                    const NodeTimeWindowKey& node_time_window_key);

  /**
   * @brief 串行阶段直写: 未命中则计算并入库
   * @throws std::logic_error 并行阶段调用
   */
  const std::vector<TimeWindow>* get_or_create_other(const NodeTimeWindowKey& node_tw_key);

  /**
   * @brief 把写缓冲的新增条目归档进本表(必须在屏障之后的串行阶段调用)
   * @details 重复键的条目直接丢弃(值语义, 无手动释放); 未重复的条目整体移入本表。
   */
  void archive(NodeTimeWindowStoreBuffer* node_tw_store_buffer);

  /**
   * @brief 统一的时间窗查找入口
   * @details 依次查本表(只读) →写缓冲(任务私有) → 计算并写入写缓冲。
   * store_buffer 为 nullptr 时表示串行阶段, 直接读写本表。
   * 返回的引用在本次推导内有效, 不得跨越归档点长期持有。
   */
  const std::vector<TimeWindow>& lookup(NodeTimeWindowStoreBuffer* node_tw_store_buffer,
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
   * @brief 表条目达到上限时回收到水位线, 给缓存一个硬内存边界
   * @details 模式键已把条目数压到“时间窗内容组合”的量级; 该上限只兜底退化输入。
   * 回收按桶序等间隔淘汰约 1/4 而不是整表清空, 让热点条目大概率保留、重算被摊薄;
   * 回收后未命中会重新计算, 结果不变。
   */
  static void recycle_if_full(
      std::unordered_map<NodeTimeWindowKey, std::vector<TimeWindow>>& table);

  /**
   * @brief 串行路径专用的 pattern id 复用缓冲, 并行阶段不得触碰
   */
  std::vector<int> serial_patterns_scratch;

  std::atomic<bool> parallel{false};
};
