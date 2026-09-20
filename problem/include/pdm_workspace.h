/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <memory>
#include <stack>
#include <vector>

#include "pdm_context.h"
#include "pdm_load.h"
#include "pdm_order_pool.h"
#include "pdm_resource.h"
#include "pdm_solution.h"

/**
 * @brief 车次和订单缓存
 */
class LoadOrderBuffer {
 public:
  using UPtr = std::unique_ptr<LoadOrderBuffer>;
  /**
   * @brief 车次列表
   */
  std::vector<Load*> loads;
  /**
   * @brief 未指派的订单集合
   */
  std::unordered_map<int, const Order*> unassigned_orders;

  LoadOrderBuffer(std::vector<Load*> _loads, std::unordered_map<int, const Order*> _unassigned_orders)
      : loads(std::move(_loads)), unassigned_orders(std::move(_unassigned_orders)) {}
};

/**
 * @brief 工作空间
 */
class Workspace {
 public:
  /**
   * @brief solver上下文
   */
  const SolverContext* context;
  /**
   * @brief 车辆资源
   */
  VehicleResource::UPtr vehicle_resource;
  /**
   * @brief 车次列表
   */
  std::vector<Load*> loads;
  /**
   * @brief 未指派的订单集合
   */
  std::unordered_map<int, const Order*> unassigned_orders;
  /**
   * @brief 车次和订单的存储栈
   */
  std::stack<LoadOrderBuffer::UPtr> load_order_stack;

  explicit Workspace(const SolverContext* _context, const OrderPool* order_pool);

  ~Workspace();

  /**
   * @brief 压入
   * @param _loads 车次集合
   * @param _unassigned_orders 未指派的订单集合
   */
  void push(const std::vector<Load*>& _loads, const std::unordered_map<int, const Order*>& _unassigned_orders);
  /**
   * @brief 弹出
   */
  void pop();
  /**
   * @brief 生成solution
   * @return solution
   */
  [[nodiscard]] Solution* generate_sol() const;
  /**
   * @brief solution覆盖workspace
   * @param sol solution
   */
  void move_solution(Solution* sol);
  /**
   * @brief 获取workspace所有load的视图
   * @return 所有load的视图
   */
  std::vector<Load*> loads_view();
  /**
   * @brief 获取workspace所有未指派order的视图
   * @return 所有未指派order的视图
   */
  std::vector<const Order*> unassigned_orders_view();
};
