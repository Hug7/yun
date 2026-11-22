/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <unordered_map>
#include <vector>

#include "bdm_time_window.h"

namespace time_window_pattern {
/**
 * @brief 订单时间窗向量(一个订单的单侧时间窗集合)的内容哈希
 */
struct Hash {
  std::size_t operator()(const std::vector<TimeWindow>& tws) const noexcept;
};

/**
 * @brief 订单时间窗向量的内容相等
 */
struct Equal {
  bool operator()(const std::vector<TimeWindow>& lhs,
                  const std::vector<TimeWindow>& rhs) const noexcept;
};
}  // namespace time_window_pattern

/**
 * @brief 订单时间窗模式登记表
 * @details 节点时间窗缓存的值只取决于节点上各订单的时间窗内容, 与订单身份、订单顺序和站点都
 * 无关。本表把一份时间窗向量登记成一个稳定的 pattern id, 于是缓存键可以从“订单组合”收窄成
 * “时间窗种类组合”, 同一时间窗内容的不同订单集合自然落到同一条缓存上。
 *
 * id 在登记表生命周期内唯一且稳定, 相同内容返回同一个 id, 不同内容返回不同 id。
 */
class TimeWindowPatternRegistry {
 public:
  /**
   * @brief 登记一组时间窗, 返回其 pattern id
   * @param tws 一个订单的单侧时间窗(提货或卸货)
   * @return 内容相同则返回同一个 id; id 从 0 开始连续分配
   */
  int intern(const std::vector<TimeWindow>& tws);

  /**
   * @brief 已登记的时间窗模式数量
   */
  [[nodiscard]] int size() const { return static_cast<int>(this->ids_.size()); }

 private:
  std::unordered_map<std::vector<TimeWindow>, int, time_window_pattern::Hash,
                     time_window_pattern::Equal>
      ids_;
};
