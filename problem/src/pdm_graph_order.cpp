/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#include "pdm_graph_order.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <ranges>
#include <vector>

#include "bdm_common.h"
#include "bdm_dist_matrix.h"
#include "bdm_location.h"
#include "pdm_load.h"
#include "pdm_order.h"
#include "prob_problem.h"

namespace {

/**
 * @brief 对订单列表排序并设置 rank 值
 * @details 分别按照 提货站点间距离 和 卸货站点间距离 排序的 rank
 * 值加和汇总，再按照汇总的rank列表排序得到最终的rank排序
 * @param elements 订单关系列表
 */
void sort_order_relationships(std::vector<OrderRelationshipElement>& elements) {
  const int element_len = static_cast<int>(elements.size());
  if (element_len == 0) {
    return;
  }

  // 对 提货 排序
  std::ranges::sort(elements,
                    [](const OrderRelationshipElement& a, const OrderRelationshipElement& b) {
                      if (a.pick_dist != b.pick_dist) {
                        return a.pick_dist < b.pick_dist;
                      }
                      return a.to_order_ind < b.to_order_ind;
                    });
  std::int64_t pre_pick_dist = elements[0].pick_dist;
  int pre_pick_rank = 0;
  for (int u = 1; u < element_len; ++u) {
    if (elements[u].pick_dist != pre_pick_dist) {
      ++pre_pick_rank;
      pre_pick_dist = elements[u].pick_dist;
    }
    elements[u].rank += pre_pick_rank;
  }

  // 对 卸货 排序
  std::ranges::sort(elements,
                    [](const OrderRelationshipElement& a, const OrderRelationshipElement& b) {
                      if (a.drop_dist != b.drop_dist) {
                        return a.drop_dist < b.drop_dist;
                      }
                      return a.to_order_ind < b.to_order_ind;
                    });
  std::int64_t pre_drop_dist = elements[0].drop_dist;
  int pre_drop_rank = 0;
  for (int u = 1; u < element_len; ++u) {
    if (elements[u].drop_dist != pre_drop_dist) {
      ++pre_drop_rank;
      pre_drop_dist = elements[u].drop_dist;
    }
    elements[u].rank += pre_drop_rank;
  }
}

}  // namespace

OrderRelationshipGraph::UPtr build_order_relationship_graph(
    const SolverContext* context, const std::vector<Order*>& orders,
    const VehicleResource::UPtr& vehicle_resource) {
  const size_t order_len = orders.size();
  auto relationship_graph = std::make_unique<OrderRelationshipGraph>(order_len);
  const auto carrier_manager = context->scenario->carrier_manager;
  const auto load_editor = context->load_editor;

  for (size_t u = 0; u < order_len; ++u) {
    const Order* from_order = orders[u];
    std::vector<OrderRelationshipElement> elements;

    for (size_t m = 0; m < order_len; ++m) {
      if (u == m) {
        continue;
      }
      const Order* to_order = orders[m];
      // 取可用车型交集
      const auto available_vehicle_bitset = BitsetUtils::call_intersection(
          from_order->available_vehicle_bitset, to_order->available_vehicle_bitset);

      if (available_vehicle_bitset->sum() == 0) {
        continue;
      }
      // 构造订单列表
      const auto orders_a_b = std::vector<const Order*>{from_order, to_order};
      const auto orders_b_a = std::vector<const Order*>{to_order, from_order};
      // 遍历车型
      const size_t bitset_len = available_vehicle_bitset->len();
      std::int64_t total_drop_dist = 0;
      std::int64_t total_pick_dist = 0;
      int eval_available_vehicle_count = 0;
      for (size_t v = 0; v < bitset_len; ++v) {
        if (available_vehicle_bitset->test(v)) {
          const auto cur_vehicle = carrier_manager->vehicles[v];
          if (!vehicle_resource->has_available(cur_vehicle)) {
            continue;
          }
          const auto cur_load_a_b = load_editor->build(orders_a_b, cur_vehicle);
          const auto cur_load_b_a = load_editor->build(orders_b_a, cur_vehicle);
          const bool infeasible = cur_load_a_b->is_infeasible() && cur_load_b_a->is_infeasible();
          delete cur_load_a_b;
          delete cur_load_b_a;
          if (infeasible) {
            continue;
          }
          ++eval_available_vehicle_count;
          const auto dist_matrix = carrier_manager->vehicles[v]->get_dist_matrix();
          total_drop_dist +=
              dist_matrix->get_dist(from_order->drop_loc->ind, to_order->drop_loc->ind);
          if (context->parameter->load_unload_policy == LoadUnloadPolicyType::FILO) {
            total_pick_dist +=
                dist_matrix->get_dist(to_order->pick_loc->ind, from_order->pick_loc->ind);
          } else {
            total_pick_dist +=
                dist_matrix->get_dist(from_order->pick_loc->ind, to_order->pick_loc->ind);
          }
        }
      }
      if (eval_available_vehicle_count > 0) {
        elements.emplace_back(from_order->ind, to_order->ind,
                              total_pick_dist / eval_available_vehicle_count,
                              total_drop_dist / eval_available_vehicle_count);
      }
    }
    // 对关联订单按照提卸货距离排序并设置 rank
    sort_order_relationships(elements);
    // 记录到 relationship_graph 中
    const auto element_len = elements.size();
    for (size_t l = 0; l < element_len; ++l) {
      relationship_graph->rows[from_order->ind].emplace(elements[l].to_order_ind, elements[l]);
    }
  }

  return relationship_graph;
}
