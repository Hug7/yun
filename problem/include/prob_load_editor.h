/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <vector>

#include "pdm_cache.h"
#include "pdm_load.h"
#include "pdm_order.h"
#include "pdm_resource.h"
#include "pdm_segment.h"

class Problem;

/**
 * @brief 车次编辑器
 * @details 对车次操作：把订单装配成车次、给车次改派车型、把订单片段插入车次。
 * 只做评估，不占用车辆资源：占用与释放由调用方通过 Solution 的 VehicleResource 完成。
 */
class LoadEditor {
 public:
  explicit LoadEditor(const Problem* problem) : problem(problem) {}

  /**
   * @brief 将订单按照顺序构造 load
   * @details 根据车辆资源选择最合适的车辆
   * @param orders 订单集合
   * @param vehicle_resource 车辆资源
   */
  [[nodiscard]] Load* build(const std::vector<const Order*>& orders,
                            const VehicleResource& vehicle_resource) const;

  /**
   * @brief 根据订单列表和指定车辆构建 Load
   * @param orders 订单列表
   * @param vehicle 车辆
   * @param store_buffer 非 nullptr 时, 本 Load 的重算只落该丢弃式写缓冲、不污染共享冻结表
   *        (预检等使用临时 Order 的场景必须传入, 否则临时 Order 释放后键悬空 -> use-after-free)
   */
  Load* build(const std::vector<const Order*>& orders, const Vehicle* vehicle,
              CacheStoreBuffer* store_buffer = nullptr) const;

  /**
   * @brief 选择最合适的车-临时解
   * @details 不占用车辆资源。改派准则：保留当前车型作为 incumbent，只有目标值严格更优的可用
   * 车型才会被换上；同分时不换车型。
   * @param load 车次
   * @param vehicle_resource 车辆资源
   */
  void select_best_vehicle(Load* load, const VehicleResource& vehicle_resource) const;

  /**
   * @brief 将 segment 贪心插入 load
   * @param ori_load origin load
   * @param segment 带顺序的订单片段
   * @param vehicle_resource 车辆资源
   * @param store_buffer 任务私有的缓存写缓冲, 会挂载到本函数创建的 load 副本上;
   * 传 nullptr 表示串行阶段(直写请求内共享的冻结表)
   * @return 插入后的 load 副本; 无可行方案时返回 nullptr
   */
  Load* greedy_insert(const Load* ori_load, const Segment* segment,
                      const VehicleResource& vehicle_resource,
                      CacheStoreBuffer* store_buffer) const;

 private:
  const Problem* problem;
};
