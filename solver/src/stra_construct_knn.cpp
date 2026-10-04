/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#include <algorithm>
#include <limits>
#include <ranges>
#include <unordered_map>
#include <utility>
#include <vector>

#include "bdm_dist_matrix.h"
#include "bdm_location.h"
#include "c_rang_utils.h"
#include "pdm_load.h"
#include "pdm_order.h"
#include "prob_problem.h"
#include "stra_construct.h"

namespace {

struct OrderPairEdge {
  /**
   * @brief 订单的索引
   */
  int order_ind;
  /**
   * @brief 平均距离
   */
  long mean_dist;

  OrderPairEdge(const int order_ind, const long mean_dist)
      : order_ind(order_ind), mean_dist(mean_dist) {}
};

/**
 * @brief 选取种子订单
 * @details 取货时间窗结束最早的订单优先成车；时间相同时取索引小的，保证结果可复现
 * @param unassigned_order_indices 未指派的订单索引集合
 * @param sol solution
 * @return 种子订单，集合为空时返回nullptr
 */
int select_seed_order_ind(std::unordered_set<int>& unassigned_order_indices, Solution* sol) {
  long seed_minimum_latest_drop_time = std::numeric_limits<long>::max();  // 最小的最晚卸货时间
  int seed_order_ind = -1;                                                // 选中的订单索引
  for (int order_ind : unassigned_order_indices) {
    const Order* order = sol->unassigned_orders[order_ind];
    if (order->drop_time_windows.empty()) {
      continue;
    }
    const long latest_drop_time = order->drop_time_windows.back()->late;

    if (latest_drop_time < seed_minimum_latest_drop_time) {
      seed_minimum_latest_drop_time = latest_drop_time;
      seed_order_ind = order_ind;
    } else if (latest_drop_time == seed_minimum_latest_drop_time && order_ind < seed_order_ind) {
      seed_order_ind = order_ind;
    }
  }
  return seed_order_ind;
}

/**
 * @brief 构造始发 order 与目的 orders 的边权重
 * @param from_order 始发 order
 * @param orders 目的 orders 列表
 * @param context solver上下文
 * @param k_neighbor_count k近邻数量
 * @return 始发 order 与目的 orders 的边权重
 */
std::unordered_map<int, int> construct_knn_edge(const Order* from_order,
                                                const std::vector<const Order*>& orders,
                                                const SolverContext* context,
                                                const int k_neighbor_count) {
  // 获取订单 提货站点间\卸货站点间 所有公共可用车型的平均距离，如果没有公共可用车型则不考虑
  const auto carrier_manager = context->scenario->carrier_manager;
  std::vector<OrderPairEdge> drop_edges;
  std::vector<OrderPairEdge> pick_edges;
  for (const auto order_b : orders) {
    if (from_order == order_b) {
      continue;
    }
    // 取可用车型交集
    const auto available_vehicle_bitset = carrier_manager->full_vehicle_bitset();
    available_vehicle_bitset->call_intersection(from_order->available_vehicle_bitset);
    available_vehicle_bitset->call_intersection(order_b->available_vehicle_bitset);

    const int vehicle_count = available_vehicle_bitset->sum();
    if (vehicle_count == 0) {
      continue;
    }
    const size_t bitset_len = available_vehicle_bitset->len();
    long total_drop_dist = 0;
    long total_pick_dist = 0;
    for (size_t u = 0; u < bitset_len; ++u) {
      if (available_vehicle_bitset->test(u)) {
        const auto dist_matrix = carrier_manager->vehicles[u]->get_dist_matrix();
        total_drop_dist += dist_matrix->get_dist(from_order->drop_loc->ind, order_b->drop_loc->ind);
        if (context->parameter->load_unload_policy == LoadUnloadPolicyType::FILO) {
          total_pick_dist +=
              dist_matrix->get_dist(order_b->pick_loc->ind, from_order->pick_loc->ind);
        } else {
          total_pick_dist +=
              dist_matrix->get_dist(from_order->pick_loc->ind, order_b->pick_loc->ind);
        }
      }
    }
    const long mean_drop_dist = total_drop_dist / vehicle_count;
    const long mean_pick_dist = total_pick_dist / vehicle_count;
    drop_edges.emplace_back(order_b->ind, mean_drop_dist);
    pick_edges.emplace_back(order_b->ind, mean_pick_dist);
  }
  if (pick_edges.empty()) {
    return {};
  }
  // 按照距离排序，从小到大
  std::ranges::sort(drop_edges, [](const OrderPairEdge& a, const OrderPairEdge& b) {
    if (a.mean_dist == b.mean_dist) {
      return a.order_ind < b.order_ind;
    }
    return a.mean_dist < b.mean_dist;
  });
  std::ranges::sort(pick_edges, [](const OrderPairEdge& a, const OrderPairEdge& b) {
    if (a.mean_dist == b.mean_dist) {
      return a.order_ind < b.order_ind;
    }
    return a.mean_dist < b.mean_dist;
  });
  // 统计提货和卸货的序号，构建rank
  std::unordered_map<int, int> ranks;
  int pick_rank_ind = 0;
  long pick_pre_rank_dist = -1;
  for (const auto& edge : pick_edges) {
    if (edge.mean_dist != pick_pre_rank_dist) {
      pick_pre_rank_dist = edge.mean_dist;
      ++pick_rank_ind;
    }
    ranks[edge.order_ind] += pick_rank_ind;
  }
  int drop_rank_ind = 0;
  long drop_pre_rank_dist = -1;
  for (const auto& edge : drop_edges) {
    if (edge.mean_dist != drop_pre_rank_dist) {
      drop_pre_rank_dist = edge.mean_dist;
      ++drop_rank_ind;
    }
    ranks[edge.order_ind] += drop_rank_ind;
  }
  // 对rank从小到大排序，取前K个不同rank值的订单Ind集合
  std::vector<std::pair<int, int>> rank_pairs;
  rank_pairs.reserve(ranks.size());
  for (auto& [fst, snd] : ranks) {
    rank_pairs.emplace_back(fst, snd);
  }
  std::ranges::sort(rank_pairs, [](const auto& pair_a, const auto& pair_b) {
    return pair_a.second < pair_b.second;
  });
  std::unordered_map<int, int> knn_order_edges;
  int pre_rank = -1;
  for (const auto& [fst, snd] : rank_pairs) {
    if (snd != pre_rank) {
      ++pre_rank;
      if (pre_rank >= k_neighbor_count) {
        break;
      }
    }
    knn_order_edges[fst] = pre_rank;
  }
  return knn_order_edges;
}

/**
 * @brief 构造K近邻图
 * @details
 * @param context solver上下文
 * @param sol solution
 * @param k_neighbor_count 近邻数
 */
std::unordered_map<int, std::unordered_map<int, int>> construct_knn_graph(
    const SolverContext* context, Solution* sol, const int k_neighbor_count) {
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
  std::unordered_map<int, std::unordered_map<int, int>> knn_graph;
  for (const auto order : orders) {
    knn_graph[order->ind] = construct_knn_edge(order, orders, context, k_neighbor_count);
  }

  return knn_graph;
}

/**
 * @brief 选择订单构造车次添加到候选车次中
 * @param candidate_load_indices 候选 load indices
 * @param unassigned_order_indices 待指派的 order indices
 * @param sol solution
 * @param problem problem
 */
void select_order_construct_load(std::unordered_set<int>& candidate_load_indices,
                                 std::unordered_set<int>& unassigned_order_indices, Solution* sol,
                                 const Problem* problem) {
  // 选择一个order构造新的load
  const size_t unassigned_order_len = unassigned_order_indices.size();
  for (size_t u = 0; u < unassigned_order_len; ++u) {
    int seed_order_ind = select_seed_order_ind(unassigned_order_indices, sol);
    if (seed_order_ind == -1) {
      return;
    }
    // 从 unassigned_order_indices 移除
    unassigned_order_indices.erase(seed_order_ind);
    // 判断seed order是否可以构造load
    const auto seed_order = sol->unassigned_orders[seed_order_ind];
    sol->remove_unassigned_order_by_key(seed_order_ind);  // 移除未指派订单
    auto cur_orders = std::vector<const Order*>{seed_order};
    Load* load = problem->construct_load_by_order(cur_orders, sol->vehicle_resource);
    if (load->is_infeasible()) {
      delete load;
    } else {
      candidate_load_indices.insert(static_cast<int>(sol->loads.size()));
      sol->add_load(load, cur_orders);
      return;
    }
  }
}

}  // namespace

// ====== implement of k_nearest_neighbor ======
void ConstructHeuristic::k_nearest_neighbor(Workspace* workspace, const KnnParameter& parameter) {
  const SolverContext* context = workspace->context;
  context->logger->info("execute knn, neighbor_count={}", parameter.neighbor_count);

  const Problem* problem = context->problem;
  auto sol = workspace->generate_sol();

  // 构造knn关系图(knn graph)
  auto knn_graph = construct_knn_graph(context, sol, parameter.neighbor_count);
  // candidate load indices
  std::unordered_set<int> candidate_load_indices =
      RangeUtils::range_set(static_cast<int>(sol->loads.size()));
  // unassigned order indices
  std::unordered_set<int> unassigned_order_indices;
  unassigned_order_indices.reserve(sol->unassigned_orders.size());
  for (const auto& order_ind : sol->unassigned_orders | std::views::keys) {
    unassigned_order_indices.insert(order_ind);
  }
  // 处理待分配的订单
  const size_t unassigned_order_len = unassigned_order_indices.size();
  for (size_t u = 0; u < unassigned_order_len; ++u) {
    // 根据 knn graph 中rank尝试将unassigned_orders逐个插入loads中；
    // 如果unassigned_orders都不能插入到loads中，那么则按照min(order最晚卸货时间)选择一个订单构造新的load，此外将没有希望的loads归档到solution中；
    // 如果load出现对所有的unassigned_orders都不可插入，那么将该load归档到solution中，减少重复计算；
    if (unassigned_order_indices.empty()) {
      break;
    }

    std::vector<int> archived_load_indices;
    int best_rank = std::numeric_limits<int>::max();
    int best_rank_for_load_ind = -1;
    int best_rank_for_order_ind = -1;
    for (const int& candidate_load_ind : candidate_load_indices) {
      const auto cur_load = sol->loads[candidate_load_ind];
      bool cur_load_can_insert = false;
      for (const int& unassigned_order_ind : unassigned_order_indices) {
        const auto& cur_order = sol->unassigned_orders[unassigned_order_ind];
        if (cur_load->last_node->prev->activity_type == ActivityType::DROP) {
          auto cur_load_last_delivery_order = cur_load->last_node->prev->last->order;
          if (knn_graph[cur_order->ind].contains(cur_load_last_delivery_order->ind)) {
            const int cur_rank = knn_graph[cur_order->ind][cur_load_last_delivery_order->ind];
            if (cur_rank > best_rank) {
              continue;
            }
            // 尝试能否将 cur_order 插入到 cur_load 的最后
            const auto tmp_load = problem->pd_pattern->deep_copy_load(cur_load);
            // 插入到最后-并且快速判断能否插入
            if (problem->pd_pattern->insert_last_delivery(tmp_load, cur_order)) {
              problem->eval_load(tmp_load);
              // 找到合适的车
              problem->tmp_select_best_vehicle(tmp_load, sol->vehicle_resource);
              if (tmp_load->is_feasible()) {
                cur_load_can_insert = true;
                if (cur_rank < best_rank) {
                  best_rank = cur_rank;
                  best_rank_for_load_ind = candidate_load_ind;
                  best_rank_for_order_ind = cur_order->ind;
                }
              }
            }
            // release tmp_load
            delete tmp_load;
          }
        }
      }
      if (!cur_load_can_insert) {
        archived_load_indices.push_back(candidate_load_ind);
      }
    }
    if (best_rank_for_order_ind == -1) {
      // 所有unassigned order不能插入，将candidate load放入solution中
      candidate_load_indices.clear();
      // 所有unassigned order不能插入，选择订单构造车次添加到候选车次中
      select_order_construct_load(candidate_load_indices, unassigned_order_indices, sol, problem);
    } else {
      // 将选中的order插入到指定的load
      auto cur_order = sol->unassigned_orders[best_rank_for_order_ind];
      // 将选中的order从unassigned_orders中移除
      unassigned_order_indices.erase(best_rank_for_order_ind);
      sol->remove_unassigned_order_by_key(best_rank_for_order_ind);
      // 选择最优车辆
      auto cur_load = sol->loads[best_rank_for_load_ind];
      problem->pd_pattern->insert_last_delivery(cur_load, cur_order);
      problem->eval_load(cur_load);
      sol->release_vehicle_resource(cur_load);
      problem->tmp_select_best_vehicle(cur_load, sol->vehicle_resource);
      sol->occupy_vehicle_resource(cur_load);
      // 将所有unassigned orders不能插入的candidate load归档
      for (int archived_load_ind : archived_load_indices) {
        candidate_load_indices.erase(archived_load_ind);
      }
    }
  }
  candidate_load_indices.clear();

  // 将solution中结果覆盖workspace
  workspace->move_solution(sol);

  context->logger->info("knn is finished. load_count={}, unassigned_order_count={}",
                        workspace->loads.size(), workspace->unassigned_orders.size());
}
