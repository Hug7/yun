/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#include <algorithm>
#include <limits>
#include <queue>
#include <ranges>
#include <unordered_map>
#include <utility>
#include <vector>

#include "bdm_dist_matrix.h"
#include "bdm_location.h"
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
 * @param unassigned_orders 未指派的订单集合
 * @return 种子订单，集合为空时返回nullptr
 */
int select_seed_order_ind(const std::unordered_map<int, const Order*>& unassigned_orders) {
  long seed_minimum_latest_drop_time = std::numeric_limits<long>::max();  // 最小的最晚卸货时间
  int seed_order_ind = -1;                                                // 选中的订单索引
  for (auto& [ind, order] : unassigned_orders) {
    if (order->drop_time_windows.empty()) {
      continue;
    }
    const long latest_drop_time = order->drop_time_windows.back()->late;

    if (latest_drop_time < seed_minimum_latest_drop_time) {
      seed_minimum_latest_drop_time = latest_drop_time;
      seed_order_ind = ind;
    } else if (latest_drop_time == seed_minimum_latest_drop_time && ind < seed_order_ind) {
      seed_order_ind = ind;
    }
  }
  return seed_order_ind;
}

std::queue<int> construct_knn_edge(const Order* from_order, const std::vector<const Order*>& orders,
                                   const SolverContext* context, const int k_neighbor_count) {
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
  for (auto [fst, snd] : ranks) {
    rank_pairs.emplace_back(fst, snd);
  }
  std::ranges::sort(rank_pairs, [](const auto& pair_a, const auto& pair_b) {
    return pair_a.second < pair_b.second;
  });
  std::queue<int> knn_order_ind_queue;
  int pre_rank = -1;
  for (const auto& rank_pair : rank_pairs) {
    if (rank_pair.second != pre_rank) {
      ++pre_rank;
      if (pre_rank >= k_neighbor_count) {
        break;
      }
    }
    knn_order_ind_queue.push(rank_pair.first);
  }
  return knn_order_ind_queue;
}

/**
 * @brief 构造K近邻图
 * @details
 * @param context solver上下文
 * @param sol solution
 * @param k_neighbor_count 近邻数
 */
std::unordered_map<int, std::queue<int>> construct_knn_graph(const SolverContext* context,
                                                             Solution* sol,
                                                             const int k_neighbor_count) {
  // 拼装所有订单
  std::vector<const Order*> orders;
  // 拼装未指派订单
  for (const auto order : sol->unassigned_orders | std::views::values) {
    orders.emplace_back(order);
  }
  // 拼装load中订单
  for (const auto load : sol->loads) {
    auto cur_orders = load->get_all_orders();
    orders.insert(orders.end(), cur_orders.begin(), cur_orders.end());
  }
  // 构造订单间的knn图
  std::unordered_map<int, std::queue<int>> knn_graph;
  for (const auto order : orders) {
    knn_graph[order->ind] = construct_knn_edge(order, orders, context, k_neighbor_count);
  }

  return knn_graph;
}

/**
 * @brief 选取距离最近的k个候选订单
 * @details SPMD模式下一个车次只有一个提货点，所以只保留提货点相同的订单；路网不可达的订单直接跳过
 * @param dist_matrix 当前车辆使用的距离矩阵
 * @param cur_loc 车次当前最后一个卸货点
 * @param pick_loc 车次的提货点
 * @param unassigned_orders 未指派的订单集合
 * @param neighbor_count 邻域大小k
 * @return 距离升序的候选订单，最多k个
 */
std::vector<const Order*> select_neighbor_orders(
    const DistMatrix* dist_matrix, const Location* cur_loc, const Location* pick_loc,
    const std::unordered_map<int, const Order*>& unassigned_orders, const int neighbor_count) {
  auto neighbors = std::vector<std::pair<long, const Order*>>();
  for (const auto& [ind, order] : unassigned_orders) {
    if (order->pick_loc != pick_loc) {
      continue;
    }
    const long dist = dist_matrix->get_dist(cur_loc->ind, order->drop_loc->ind);
    if (dist >= DistMatrixParameter::MAX_DISTANCE) {
      continue;
    }
    neighbors.emplace_back(dist, order);
  }
  std::ranges::sort(neighbors, [](const auto& neighbor_a, const auto& neighbor_b) {
    if (neighbor_a.first != neighbor_b.first) {
      return neighbor_a.first < neighbor_b.first;
    }
    return neighbor_a.second->ind < neighbor_b.second->ind;
  });

  // k<=0时不做扩张
  const size_t neighbor_len =
      neighbor_count > 0 ? std::min(static_cast<size_t>(neighbor_count), neighbors.size()) : 0;
  auto candidate_orders = std::vector<const Order*>(neighbor_len);
  for (size_t u = 0; u < neighbor_len; ++u) {
    candidate_orders[u] = neighbors[u].second;
  }
  return candidate_orders;
}
}  // namespace

// ====== implement of k_nearest_neighbor ======
void ConstructHeuristic::k_nearest_neighbor(Workspace* workspace, const KnnParameter& parameter) {
  const SolverContext* context = workspace->context;
  const Problem* problem = context->problem;
  auto sol = workspace->generate_sol();

  if (sol->loads.empty()) {
    // 选择一个order构造新的load
    const size_t unassigned_order_len = sol->unassigned_orders.size();
    for (size_t u = 0; u < unassigned_order_len; ++u) {
      int seed_order_ind = select_seed_order_ind(sol->unassigned_orders);
      if (seed_order_ind == -1) {
        break;
      }
      // 判断seed order是否可以构造load
      const auto seed_order = sol->unassigned_orders[seed_order_ind];
      sol->remove_unassigned_order_by_key(seed_order_ind);
      auto cur_orders = std::vector<const Order*>{seed_order};
      Load* load = problem->construct_load_by_order(cur_orders);
      if (load->is_infeasible()) {
        delete load;
        sol->add_unassigned_order(seed_order);
      } else {
        sol->add_load(load);
        break;
      }
    }
  }

  // 构造knn关系图
  auto knn_graph = construct_knn_graph(context, sol, parameter.neighbor_count);

  const size_t unassigned_order_len = sol->unassigned_orders.size();
  for (size_t u = 0; u < unassigned_order_len; ++u) {
    //
  }

  context->logger->info("execute knn, neighbor_count={}", parameter.neighbor_count);
  // 单车不可行的订单先移出未指派集合，构造结束后再放回，避免种子选取陷入死循环
  // auto unservable_orders = std::vector<Order*>();
  //
  // while (!workspace->unassigned_orders.empty()) {
  //   // step 1: 用种子订单构造新车次，车辆由problem挑选最优的一个
  //   Order* seed_order = select_seed_order(workspace->unassigned_orders);
  //   workspace->unassigned_orders.erase(seed_order->ind);
  //   auto assigned_orders = std::vector<Order*>({seed_order});
  //   Load* load = problem->construct_load_by_order(assigned_orders);
  //   if (load->constr_profile->is_infeasible()) {
  //     delete load;
  //     unservable_orders.push_back(seed_order);
  //     continue;
  //   }
  //   const DistMatrix* dist_matrix = load->vehicle->get_dist_matrix();
  //   const Location* pick_loc = seed_order->pick_loc;
  //
  //   // step 2: 最近邻扩张，k个近邻都插不进去时本车次结束
  //   while (true) {
  //     const auto candidate_orders =
  //         select_neighbor_orders(dist_matrix, assigned_orders.back()->drop_loc, pick_loc,
  //                                workspace->unassigned_orders, parameter.neighbor_count);
  //     Load* next_load = nullptr;
  //     Order* next_order = nullptr;
  //     for (auto candidate_order : candidate_orders) {
  //       auto trial_orders = assigned_orders;
  //       trial_orders.push_back(candidate_order);
  //       // 试算车次：可行则整体采用，不可行则整体丢弃，避免回滚已经改动的节点链
  //       Load* trial_load = problem->construct_load_by_order(trial_orders, load->vehicle);
  //       if (trial_load->constr_profile->is_feasible()) {
  //         next_load = trial_load;
  //         next_order = candidate_order;
  //         break;
  //       }
  //       delete trial_load;
  //     }
  //     if (next_load == nullptr) {
  //       break;
  //     }
  //     delete load;
  //     load = next_load;
  //     assigned_orders.push_back(next_order);
  //     workspace->unassigned_orders.erase(next_order->ind);
  //   }
  //
  //   // step 3: 车次落入工作空间
  //   context->logger->debug("knn load is constructed. vehicle={}, order_count={}, dist={}m",
  //                          load->vehicle->ind, assigned_orders.size(), load->get_total_dist());
  //   workspace->loads.push_back(load);
  // }
  //
  // // 不可行的订单放回未指派集合，由solution导出
  // for (auto unservable_order : unservable_orders) {
  //   workspace->unassigned_orders.emplace(unservable_order->ind, unservable_order);
  // }
  // context->logger->info("knn is finished. load_count={}, unassigned_order_count={}",
  //                       workspace->loads.size(), workspace->unassigned_orders.size());
}
