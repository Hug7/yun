/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <unordered_map>

#include "pdm_graph_order.h"
#include "pdm_solution.h"

/**
 * @brief 订单 K 近邻图
 * @details 语义为 graph[from_order_ind][to_order_ind] = rank；rank 越小越近，
 *          rank 在 [0, k_neighbor_count) 范围内。图中包含解里所有订单
 *          （未指派订单与车次中的订单）。
 */
using KnnGraph = std::unordered_map<int, std::unordered_map<int, int>>;

/**
 * @brief 构造解中订单的 K 近邻图
 * @param order_relationship_graph 订单关系图
 * @param sol solution
 * @param k_neighbor_count 近邻数
 * @return K 近邻图，键与值均为订单索引
 */
[[nodiscard]] KnnGraph construct_knn_graph(const OrderRelationshipGraph& order_relationship_graph,
                                           const Solution* sol, int k_neighbor_count);
