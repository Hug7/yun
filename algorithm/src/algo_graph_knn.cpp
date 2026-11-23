/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#include "algo_graph_knn.h"

#include <ranges>
#include <unordered_map>
#include <vector>

#include "pdm_load.h"
#include "pdm_order.h"

KnnGraph construct_knn_graph(const OrderRelationshipGraph& order_relationship_graph,
                             const Solution* sol, const int k_neighbor_count) {
  // 拼装所有订单
  std::vector<const Order*> orders;

  // 拼装未指派订单
  for (const auto order : sol->unassigned_orders | std::views::values) {
    orders.emplace_back(order);
  }
  // 拼装load中订单
  for (const auto load : sol->loads) {
    auto cur_orders = load->get_orders();
    orders.insert(orders.end(), cur_orders.begin(), cur_orders.end());
  }
  // 构造订单间的knn图
  KnnGraph knn_graph;
  for (const auto from_order : orders) {
    int from_order_ind = from_order->ind;
    auto cols = order_relationship_graph.rows[from_order_ind];
    std::vector<OrderRelationshipElement*> elements;
    elements.reserve(k_neighbor_count);

    for (const auto to_order : orders) {
      const int to_order_ind = to_order->ind;
      if (from_order_ind != to_order_ind) {
        if (cols.contains(to_order_ind)) {
          elements.emplace_back(&cols[to_order_ind]);
        }
      }
    }
    std::ranges::sort(elements,
                      [](const OrderRelationshipElement* a, const OrderRelationshipElement* b) {
                        if (a->rank != b->rank) {
                          return a->rank < b->rank;
                        }
                        return a->to_order_ind < b->to_order_ind;
                      });

    const size_t element_len = elements.size();
    if (element_len == 0) {
      continue;
    }
    int pre_rank = -1;
    int pre_element_rank = -1;
    for (size_t e = 0; e < element_len; ++e) {
      const OrderRelationshipElement* element = elements[e];
      if (pre_element_rank != element->rank) {
        pre_element_rank = element->rank;
        ++pre_rank;
        if (pre_rank >= k_neighbor_count) {
          break;
        }
      }
      knn_graph[from_order_ind].emplace(element->to_order_ind, pre_rank);
    }
  }

  return knn_graph;
}
