/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <vector>

#include "bdm_cargo_order.h"
#include "cc_manager.h"
#include "hc_manager.h"
#include "pdm_infeasible_order.h"
#include "pdm_load.h"
#include "pdm_order.h"
#include "pdm_parameter.h"
#include "pdm_resource.h"
#include "prob_pattern.h"
#include "prob_policy.h"
#include "sc_manager.h"
#include "se_scenario.h"

class Problem {
 public:
  const Scenario* scenario;

  Parameter* parameter;

  LoadUnloadPolicy* lu_policy;

  PickDropPattern* pd_pattern;

  LoadContext* load_context;

  HardConstraintManager* hc_manager;

  SoftConstraintManager* sc_manager;

  CostConstraintManager* cc_manager;

  Problem(const Scenario* scenario, Parameter* parameter);

  virtual ~Problem();

  /**
   * @brief evaluate all constraints
   */
  virtual void eval_load(Load* load) const;

  virtual LoadConstrProfile::UPtr tmp_eval_load(Load* load) const;
  /**
   * @brief 选择最合适的车-临时解
   * @details 不占用车辆资源
   * @param load 车次
   * @param vehicle_resource 车辆资源
   */
  virtual void tmp_select_best_vehicle(Load* load, VehicleResource::UPtr& vehicle_resource) const;
  /**
   * @brief check cargo order feasibility
   */
  virtual InfeasibleCargoOrder::UPtr check_feasibility(CargoOrder* cargo_order) const;
  /**
   * @brief 将订单按照顺序构造load
   * @details 根据车辆资源选择最合适的车辆，不支持PDP场景
   * @param orders 订单集合
   * @param vehicle_resource 车辆资源
   */
  virtual Load* construct_load_by_order(std::vector<const Order*>& orders,
                                        VehicleResource::UPtr& vehicle_resource) const;
  /**
   * @brief 根据订单列表和车辆构建 Load
   * @param orders 订单列表
   * @param vehicle 车辆
   * @param store_buffer 非 nullptr 时, 本 Load 的时间窗重算只落该丢弃式写缓冲、不污染共享冻结表
   *        (预检等使用临时 Order 的场景必须传入, 否则临时 Order 释放后键悬空 -> use-after-free)
   */
  virtual Load* construct_load_by_order(std::vector<const Order*>& orders, Vehicle* vehicle,
                                        CacheStoreBuffer* store_buffer = nullptr) const;
  /**
   * @brief 将 segment 贪心插入 load
   * @param ori_load origin load
   * @param segment 带顺序的订单片段
   * @param vehicle_resource 车辆资源
   * @param store_buffer 任务私有的缓存写缓冲, 会挂载到本函数创建的 load 副本上;
   * 传 nullptr 表示串行阶段(直写请求内共享的冻结表)
   * @return 插入后的 load 副本; 无可行方案时返回 nullptr
   */
  virtual Load* greedy_insert_segment_to_load(Load* ori_load, Segment* segment,
                                              VehicleResource::UPtr& vehicle_resource,
                                              CacheStoreBuffer* store_buffer) const;
};
