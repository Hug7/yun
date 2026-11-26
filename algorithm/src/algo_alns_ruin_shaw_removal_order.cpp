/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#include "algo_alns_ruin_shaw_removal_order.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <format>
#include <limits>
#include <numeric>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "bdm_time_window_utils.h"
#include "pdm_activity.h"
#include "pdm_graph_order.h"
#include "pdm_order.h"

// ====== implement of min_max_normalize ======
std::vector<double> min_max_normalize(const std::vector<double>& values) {
  std::vector<double> res(values.size(), 0.0);
  // 有限值的极值: 非有限值(无穷大/NaN)不参与, 单独映射为 1.0
  bool has_finite = false;
  double finite_min = 0.0;
  double finite_max = 0.0;
  for (const double value : values) {
    if (!std::isfinite(value)) {
      continue;
    }
    if (!has_finite) {
      finite_min = value;
      finite_max = value;
      has_finite = true;
      continue;
    }
    finite_min = std::min(finite_min, value);
    finite_max = std::max(finite_max, value);
  }
  // 全是无穷大: 无法区分, 统一按最远处理
  if (!has_finite) {
    std::ranges::fill(res, 1.0);
    return res;
  }
  const double span = finite_max - finite_min;
  for (std::size_t u = 0; u < values.size(); ++u) {
    const double value = values[u];
    if (!std::isfinite(value)) {
      res[u] = 1.0;
    } else if (span <= 0.0) {
      // 有限值全部相等: 没有区分度
      res[u] = 0.0;
    } else {
      res[u] = (value - finite_min) / span;
    }
  }
  return res;
}

// ====== implement of select_shaw_removal_order_indices ======
std::vector<int> select_shaw_removal_order_indices(int order_count,
                                                   const ShawOrderDistanceFunc& order_distance,
                                                   int count, double spatial_weight,
                                                   double time_weight, double noise_exponent,
                                                   RandomUtils& random) {
  std::vector<int> selected;
  if (order_count <= 0 || count <= 0) {
    return selected;
  }
  // 候选池: 初始化后按局部下标升序, 同时作为同距离时的可复现次序
  std::vector<int> pool(static_cast<std::size_t>(order_count));
  std::iota(pool.begin(), pool.end(), 0);

  // 随机种子订单
  const int seed_pos = random.random_int(0, order_count - 1);
  const int seed_order = pool[seed_pos];
  selected.reserve(static_cast<std::size_t>(std::min(count, order_count)));
  selected.push_back(seed_order);
  pool.erase(pool.begin() + seed_pos);

  // 每个候选到已选集合的最小空间/时间距离
  std::vector<double> spatial_dists(pool.size(), std::numeric_limits<double>::infinity());
  std::vector<double> time_dists(pool.size(), std::numeric_limits<double>::infinity());
  for (std::size_t u = 0; u < pool.size(); ++u) {
    const auto [spatial_dist, time_dist] = order_distance(pool[u], seed_order);
    spatial_dists[u] = spatial_dist;
    time_dists[u] = time_dist;
  }

  while (static_cast<int>(selected.size()) < count && !pool.empty()) {
    const std::vector<double> normalized_spatial = min_max_normalize(spatial_dists);
    const std::vector<double> normalized_time = min_max_normalize(time_dists);
    std::vector<std::size_t> ranked(pool.size());
    std::iota(ranked.begin(), ranked.end(), 0);
    std::vector<double> combined(pool.size(), 0.0);
    for (std::size_t u = 0; u < pool.size(); ++u) {
      combined[u] = spatial_weight * normalized_spatial[u] + time_weight * normalized_time[u];
    }
    std::ranges::sort(ranked, [&combined, &pool](const std::size_t lhs, const std::size_t rhs) {
      if (combined[lhs] != combined[rhs]) {
        return combined[lhs] < combined[rhs];
      }
      return pool[lhs] < pool[rhs];
    });
    // 按 `pos = floor(|pool| * y^p)` 取位: p 越大越集中在最相近者
    const double random_value = random.random_double();
    const double scaled =
        static_cast<double>(ranked.size()) * std::pow(random_value, noise_exponent);
    const std::size_t pos = std::min(ranked.size() - 1, static_cast<std::size_t>(scaled));
    const std::size_t chosen_pos = ranked[pos];
    const int chosen_order = pool[chosen_pos];
    selected.push_back(chosen_order);

    // 从候选池移除已选订单, 并把新成员的距离并入每个候选的最小距离
    pool.erase(pool.begin() + static_cast<std::ptrdiff_t>(chosen_pos));
    spatial_dists.erase(spatial_dists.begin() + static_cast<std::ptrdiff_t>(chosen_pos));
    time_dists.erase(time_dists.begin() + static_cast<std::ptrdiff_t>(chosen_pos));
    for (std::size_t u = 0; u < pool.size(); ++u) {
      const auto [spatial_dist, time_dist] = order_distance(pool[u], chosen_order);
      if (spatial_dist < spatial_dists[u]) {
        spatial_dists[u] = spatial_dist;
      }
      if (time_dist < time_dists[u]) {
        time_dists[u] = time_dist;
      }
    }
  }

  return selected;
}

// ====== implement of AlnsRuinShawRemovalOrder ======
AlnsRuinShawRemovalOrder::AlnsRuinShawRemovalOrder(
    const SolverContext* _context, const OrderRelationshipGraph* _order_relationship_graph,
    const AlnsRuinShawRemovalOrderParam& _parameter)
    : AlnsRuinOperator(AlnsRuinOperatorCode::RuinShawRemovalOrder, _context),
      parameter(_parameter),
      order_relationship_graph(_order_relationship_graph) {
  this->select_order_random =
      std::make_unique<RandomUtils>(this->parameter.select_order_random_seed);

  // 校验参数
  if (this->parameter.select_order_min_rate < 0.0 || this->parameter.select_order_min_rate > 1.0) {
    throw std::invalid_argument(this->code + ": `select_order_min_rate` must be in [0, 1], got " +
                                std::to_string(this->parameter.select_order_min_rate));
  }

  if (this->parameter.select_order_max_rate < 0.0 || this->parameter.select_order_max_rate > 1.0) {
    throw std::invalid_argument(this->code + ": `select_order_max_rate` must be in [0, 1], got " +
                                std::to_string(this->parameter.select_order_max_rate));
  }

  if (this->parameter.select_order_min_rate > this->parameter.select_order_max_rate) {
    throw std::invalid_argument(this->code + ": `select_order_min_rate` (" +
                                std::to_string(this->parameter.select_order_min_rate) +
                                ") must not exceed select_order_max_rate (" +
                                std::to_string(this->parameter.select_order_max_rate) + ")");
  }

  if (this->parameter.select_order_noise_exponent < 1.0) {
    throw std::invalid_argument(this->code + ": `select_order_noise_exponent` must be >= 1, got " +
                                std::to_string(this->parameter.select_order_noise_exponent));
  }

  if (this->parameter.select_order_spatial_weight < 0.0 ||
      this->parameter.select_order_time_weight < 0.0) {
    throw std::invalid_argument(
        this->code + ": `select_order_spatial_weight` and `select_order_time_weight` must be >= 0");
  }

  if (this->parameter.select_order_spatial_weight == 0.0 &&
      this->parameter.select_order_time_weight == 0.0) {
    throw std::invalid_argument(
        this->code +
        ": `select_order_spatial_weight` and `select_order_time_weight` must not both be 0");
  }
}

std::string AlnsRuinShawRemovalOrder::format_parameters() const {
  return std::format(
      "select_order_random_seed={}, select_order_min_rate={}, select_order_max_rate={}, "
      "select_order_noise_exponent={}, select_order_spatial_weight={}, "
      "select_order_time_weight={}",
      this->parameter.select_order_random_seed, this->parameter.select_order_min_rate,
      this->parameter.select_order_max_rate, this->parameter.select_order_noise_exponent,
      this->parameter.select_order_spatial_weight, this->parameter.select_order_time_weight);
}

std::unordered_map<int, AlnsRuinLoadActivity::UPtr> AlnsRuinShawRemovalOrder::execute(
    Solution* sol) {
  std::unordered_map<int, AlnsRuinLoadActivity::UPtr> select_order_res;
  // 遍历解中所有的activity, 收集每个订单的提货activity: (所属load索引, pick activity)
  std::vector<std::pair<int, Activity*>> pick_activities;
  // 与 pick_activities 同序的订单, 供相近度按局部下标取订单
  std::vector<const Order*> order_of_pick;
  pick_activities.reserve(sol->orders_view.size());
  order_of_pick.reserve(sol->orders_view.size());
  const int load_count = static_cast<int>(sol->loads.size());
  for (int load_ind = 0; load_ind < load_count; ++load_ind) {
    for (const auto& activity : sol->loads[load_ind]->get_pick_activities()) {
      pick_activities.emplace_back(load_ind, activity);
      order_of_pick.emplace_back(activity->order);
    }
  }
  // 没有要处理的订单
  const int order_count = static_cast<int>(pick_activities.size());
  if (order_count == 0) {
    return select_order_res;
  }
  // 随机确定要移除的订单数
  const int select_order_count = std::max(
      1, this->select_order_random->random_int(order_count, this->parameter.select_order_min_rate,
                                               this->parameter.select_order_max_rate));
  // 订单对相近度: 空间取关系图的提货+卸货距离, 时间取提卸时间窗的最近间隙
  const OrderRelationshipGraph* graph = this->order_relationship_graph;
  const ShawOrderDistanceFunc order_distance = [graph, &order_of_pick](const int local_a,
                                                                       const int local_b) {
    const Order* order_a = order_of_pick[local_a];
    const Order* order_b = order_of_pick[local_b];
    const std::int64_t time_dist =
        TimeWindowUntils::gap(order_a->pick_time_windows, order_b->pick_time_windows) +
        TimeWindowUntils::gap(order_a->drop_time_windows, order_b->drop_time_windows);
    // 无关系图: 空间项按 0 处理, 等价于只用时间项
    if (graph == nullptr) {
      return std::pair<double, double>{0.0, static_cast<double>(time_dist)};
    }
    // 关系图按订单 ind 建行; 先查 a→b, 再回退 b→a
    const std::size_t row_count = graph->rows.size();
    const auto find_element = [graph, row_count](
                                  const int from_ind,
                                  const int to_ind) -> const OrderRelationshipElement* {
      if (from_ind < 0 || static_cast<std::size_t>(from_ind) >= row_count) {
        return nullptr;
      }
      const auto& row = graph->rows[static_cast<std::size_t>(from_ind)];
      const auto it = row.find(to_ind);
      return it == row.end() ? nullptr : &it->second;
    };
    const OrderRelationshipElement* element = find_element(order_a->ind, order_b->ind);
    if (element == nullptr) {
      element = find_element(order_b->ind, order_a->ind);
    }
    // 查不到记录: 两个订单不可拼车, 视为最远
    if (element == nullptr) {
      return std::pair<double, double>{std::numeric_limits<double>::infinity(),
                                       static_cast<double>(time_dist)};
    }
    const std::int64_t spatial_dist = element->pick_dist + element->drop_dist;
    return std::pair<double, double>{static_cast<double>(spatial_dist),
                                     static_cast<double>(time_dist)};
  };
  const std::vector<int> selected_orders = select_shaw_removal_order_indices(
      order_count, order_distance, select_order_count, this->parameter.select_order_spatial_weight,
      this->parameter.select_order_time_weight, this->parameter.select_order_noise_exponent,
      *this->select_order_random);
  // 按所属load归集待移除的activity, 提卸成对移除
  for (const int local_ind : selected_orders) {
    const auto [load_ind, pick_activity] = pick_activities[local_ind];
    auto& load_activities = select_order_res[load_ind];
    if (load_activities == nullptr) {
      load_activities = std::make_unique<AlnsRuinLoadActivity>(load_ind);
    }
    load_activities->activities.emplace_back(pick_activity);
    load_activities->activities.emplace_back(pick_activity->related);
  }

  return select_order_res;
}
