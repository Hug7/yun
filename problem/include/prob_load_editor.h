/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <cstdint>
#include <optional>
#include <vector>

#include "pdm_cache.h"
#include "pdm_load.h"
#include "pdm_order.h"
#include "pdm_resource.h"
#include "pdm_segment.h"

class Problem;

/**
 * @brief 一次 segment 插入的完整计划
 * @details 由 evaluate_insert 产出、apply_insert 落实: 不携带 Load 信息
 */
struct InsertPlan {
  /**
   * @brief 插入槽位在 enumerate_insert_slots 结果中的下标
   */
  int slot_ind{-1};
  /**
   * @brief 落实方案使用的车型
   */
  const Vehicle* vehicle{nullptr};
  /**
   * @brief 落实后的目标值(绝对目标值, 非 delta)
   */
  double obj_val{0.0};
};

/**
 * @brief 单个车型对同一插入动作的评估快照
 * @details 供 evaluate_insert 选车: 先把所有车型各评估一次, 再按成本升序挑选
 */
struct VehicleInsertSnapshot {
  /**
   * @brief 车型
   */
  const Vehicle* vehicle{nullptr};
  /**
   * @brief 车型在 VehicleResource::vehicle_usages 中的下标
   */
  int vehicle_ind{-1};
  /**
   * @brief 换成该车型后插入是否可行
   */
  bool feasible{false};
  /**
   * @brief 可行时的目标值; 不可行时为 +inf, 排序时落在最后
   */
  double obj_val{0.0};
  /**
   * @brief 该车型当前是否还有空位
   */
  bool has_resource{false};
};

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
   * @param orders 订单列表
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
   * @brief 为当前插入动作采集所有车型的评估快照
   * @details 逐个车型 change_vehicle + eval_load, 记录是否可行、目标值、是否还有空位,
   * 再按 obj_val 升序稳定排序(不可行排在最后)。只评估、不占用资源; 结束时把 load 恢复成
   * 调用前的车型(不重新 eval, 由调用方决定最终车型并 eval)。
   * 预置条件: 调用前 load 已按当前车型 eval 过(insert 已落到车次上)。
   * @param load 已插入 segment 的车次副本
   * @param vehicle_resource 车辆资源
   * @return 按成本升序的全车型快照(含当前车型)
   */
  [[nodiscard]] std::vector<VehicleInsertSnapshot> snapshot_vehicles(
      Load* load, const VehicleResource& vehicle_resource) const;

  /**
   * @brief 按成本升序选当前可用的最便宜车型, 应用到 load
   * @details 单次扫描, 不构造快照也不排序: 原车型即使已无空位也算可用, 其余车型要求有空位;
   * 成本打平时保留原车型; 没有可用车型时保留原车型并重新 eval,
   * 由调用方通过 load->is_feasible() 判断是否可接受
   * @pre 调用前 load 已按当前车型 eval 过, 函数据此取原车型的成本作初值
   * @return 选中的车型; 没有可用车型时返回 nullptr
   */
  const Vehicle* select_current_best_vehicle(Load* load,
                                             const VehicleResource& vehicle_resource) const;

  /**
   * @brief 评估把 segment 贪心插入 load 的最优方案
   * @details 只试插、不回写 ori_load; 内部在 ori_load 的副本上枚举槽位并挑选最优车型,
   * 返回的计划不携带 Load 指针, 可在稍后由 apply_insert 落实
   * @param ori_load origin load
   * @param segment 带顺序的订单片段
   * @param vehicle_resource 车辆资源
   * @param store_buffer 任务私有的缓存写缓冲, 会挂载到评估用的临时 load 副本上;
   * 传 nullptr 表示串行阶段(直写请求内共享的冻结表)
   * @param watched_vehicles 非空时写入位图: 成本更低、当前可行、但因车型被占满而没有空位的
   * 车型下标。使用桶合并，位为 1 表示"该车型或同桶车型一空出, 这一格的结论可能变好", 由
   * RepairMatrix 记账并在释放时定点重评估。位图按下标 % 64 分桶, 车型数超过 64 时同桶共用一位
   * @return 最优插入计划; 无可行方案时返回 std::nullopt
   */
  [[nodiscard]] std::optional<InsertPlan> evaluate_insert(
      const Load* ori_load, const Segment* segment, const VehicleResource& vehicle_resource,
      CacheStoreBuffer* store_buffer, std::uint64_t* watched_vehicles = nullptr) const;

  /**
   * @brief 落实一个由 evaluate_insert 产出的插入计划
   * @details 在 ori_load 的新副本上重做一次插入; 计划槽位越界或落实后仍不可行时返回 nullptr
   * @param ori_load origin load
   * @param segment 带顺序的订单片段
   * @param plan 插入计划; slot_ind 索引本次 enumerate_insert_slots 的结果
   * @param store_buffer 任务私有的缓存写缓冲, 会挂载到新 load 副本上
   * @return 新的 load(所有权转移给调用方); 落实失败时返回 nullptr
   */
  [[nodiscard]] Load* apply_insert(const Load* ori_load, const Segment* segment,
                                   const InsertPlan& plan, CacheStoreBuffer* store_buffer) const;

  /**
   * @brief 将 segment 贪心插入 load
   * @details 等价于 evaluate_insert + apply_insert; 用于不保留计划的调用路径
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
