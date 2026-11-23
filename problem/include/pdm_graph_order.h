/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <cstdint>
#include <memory>
#include <unordered_map>
#include <vector>

#include "pdm_context.h"

struct OrderRelationshipElement {
  int from_order_ind{-1};
  int to_order_ind{-1};
  std::int64_t pick_dist{-1};
  std::int64_t drop_dist{-1};
  int rank{0};
};

class OrderRelationshipGraph {
 public:
  using UPtr = std::unique_ptr<OrderRelationshipGraph>;
  /**
   * @brief 订单之间关联关系
   * @details 包含内容:是否可以拼车、地理上的关联度排序。
   */
  std::vector<std::unordered_map<int, OrderRelationshipElement>> rows;
  /**
   * @brief 订单数
   */
  int order_len;

  explicit OrderRelationshipGraph(const size_t len) : rows(len), order_len(static_cast<int>(len)) {}
};

/**
 * @brief 构建订单关系图
 * @param context solver上下文
 * @param orders 订单列表
 * @param vehicle_resource 车辆资源
 */
OrderRelationshipGraph::UPtr build_order_relationship_graph(
    const SolverContext* context, const std::vector<Order*>& orders,
    const VehicleResource::UPtr& vehicle_resource);
