/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#include "pdm_order_pool.h"

#include <algorithm>
#include <memory>
#include <ranges>
#include <unordered_map>
#include <unordered_set>
#include <utility>

#include "bdm_cargo_order.h"

/**
 * @brief 根据提货点和卸货点对cargo order集合分组
 * @param cargo_orders 订单集合
 * @param location_manager 站点
 * @return
 */
std::vector<std::vector<CargoOrder*>> cargo_orders_group_by_loc(
    const std::vector<CargoOrder*>& cargo_orders, const LocationManager* location_manager) {
  const int loc_len = location_manager->len;
  std::vector<std::vector<CargoOrder*>> cargo_order_groups;
  std::unordered_map<int, int> cargo_order_group_ind_map;
  int group_ind = -1;
  for (const auto cargo_order : cargo_orders) {
    int ind_key = cargo_order->pick_loc->ind * loc_len + cargo_order->drop_loc->ind;
    auto it = cargo_order_group_ind_map.find(ind_key);
    if (it != cargo_order_group_ind_map.end()) {
      cargo_order_groups[it->second].push_back(cargo_order);
    } else {
      ++group_ind;
      cargo_order_groups.push_back(std::vector<CargoOrder*>({cargo_order}));
      cargo_order_group_ind_map[ind_key] = group_ind;
    }
  }

  return cargo_order_groups;
}

/**
 * @brief 按照label对cargo order分组再次
 * @param cargo_order_groups cargo order分组
 * @param cargo_order_group_rule cargo order分组规则
 * @return
 */
std::vector<std::vector<CargoOrder*>> cargo_orders_group_by_label(
    std::vector<std::vector<CargoOrder*>> cargo_order_groups,
    const CargoOrderGroupRule* cargo_order_group_rule) {
  if (cargo_order_group_rule->label == nullptr) {
    return cargo_order_groups;
  }
  const int label_ind4labelset = cargo_order_group_rule->label_ind4labelset;

  auto label_value_ind_groups =
      std::vector<std::unordered_set<int>>(cargo_order_group_rule->label_value_ind_groups.begin(),
                                           cargo_order_group_rule->label_value_ind_groups.end());
  std::vector<std::vector<CargoOrder*>> res_cargo_order_groups;
  // key = index of label_value_ind_groups, value = index of res_cargo_order_groups
  std::unordered_map<size_t, size_t> ind_map;

  for (auto& cargo_order_group : cargo_order_groups) {
    // todo 按照 cargo_order_group_rule->label_value_groups 分组
    for (auto& cargo_order : cargo_order_group) {
      std::unordered_set<int> label_value_indices;
      for (const auto& sub_order : cargo_order->sub_orders) {
        const auto& label_values = sub_order->labelset_value->label_values[label_ind4labelset];
        for (const auto& label_value_ind : label_values | std::views::keys) {
          label_value_indices.insert(label_value_ind);
        }
      }
      // 判断属于哪个分组
      // --为空的场景
      if (label_value_indices.empty()) {
        if (!ind_map.contains(-1)) {
          ind_map.emplace(-1, res_cargo_order_groups.size());
          res_cargo_order_groups.emplace_back();
        }
        res_cargo_order_groups[ind_map[-1]].push_back(cargo_order);
      }
      // --判断为哪个分组的子集
      const size_t tmp_len = label_value_ind_groups.size();
      bool find_subset_flag = false;
      for (size_t u = 0; u < tmp_len; ++u) {
        auto cur_label_value_indices = label_value_ind_groups[u];
        if (std::ranges::includes(cur_label_value_indices, label_value_indices)) {
          if (!ind_map.contains(u)) {
            ind_map[u] = res_cargo_order_groups.size();
            res_cargo_order_groups.emplace_back();
          }
          res_cargo_order_groups[ind_map[u]].push_back(cargo_order);
          find_subset_flag = true;
          break;
        }
      }
      if (!find_subset_flag) {
        ind_map[label_value_ind_groups.size()] = res_cargo_order_groups.size();
        // 记录新的 label value indices
        label_value_ind_groups.push_back(label_value_indices);
        res_cargo_order_groups.emplace_back();
        res_cargo_order_groups[res_cargo_order_groups.size() - 1].push_back(cargo_order);
      }
    }
  }

  return res_cargo_order_groups;
}

/**
 * @brief 将cargo orders合并成order
 * @details
 * @param cargo_orders cargo order列表
 * @param order_generate_index order索引管理器
 * @param context solver context
 * @return 合并后的order
 */
std::vector<Order*> merge_cargo_orders_to_orders(
    std::vector<CargoOrder*>& cargo_orders,
    const std::unique_ptr<GenerateIndex>& order_generate_index, const SolverContext* context) {
  const int ind = order_generate_index->next();
  Order* order = OrderFactory::creat_order(
      cargo_orders, ind, context->parameter->plan_datetime_range, context->scenario);
  // todo 需要校验合并后是否有解，无解后要再次对cargo order分组，要尽量减少分组数
  return {order};
}

// ====== implement of OrderPool ======
OrderPool::OrderPool(const SolverContext* context) : orders(), len(0) {
  /**
   * 合并cargo order规则
   * 1. cargo order的提货站点和卸货站点必须相同；
   * 2. 如果设置按照label合并，支持两种合并模式：
   * 2.1 按照label value种类分组，e.g. [A] [B] [C] [D]；
   * 2.2 指定label value组合合并, e.g. [A, B], [C, D]；
   * 3. 最重要的合并后要有可行解，要能被最小可用车型配送，也就是说合并变成最小化分组数量的多背包问题
   */

  // step 1: 按照提货和卸货站点对cargo order集合分组
  auto cargo_order_groups = cargo_orders_group_by_loc(
      context->scenario->cargo_order_manager->cargo_orders, context->scenario->location_manager);
  // step 2: 按照label对cargo order分组进行细分
  cargo_order_groups = cargo_orders_group_by_label(std::move(cargo_order_groups),
                                                   context->parameter->cargo_order_group_rule);
  // step 3: 合并cargo order
  const auto order_generate_index = std::make_unique<GenerateIndex>();  // 订单索引生成器
  for (auto& cargo_orders : cargo_order_groups) {
    auto tmp_cargo_orders =
        merge_cargo_orders_to_orders(cargo_orders, order_generate_index, context);
    this->orders.insert(this->orders.end(), tmp_cargo_orders.begin(), tmp_cargo_orders.end());
  }
  this->len = static_cast<int>(this->orders.size());
}

OrderPool::~OrderPool() {
  for (const auto& order : this->orders) {
    delete order;
  }
}