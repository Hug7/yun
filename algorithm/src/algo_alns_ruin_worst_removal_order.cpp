/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#include "algo_alns_ruin_worst_removal_order.h"

#include <algorithm>
#include <cmath>
#include <format>
#include <memory>
#include <numeric>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "c_constant.h"

namespace {

/**
 * @brief 从 node 的 activity 链表中摘除单个 activity, 并保留其所有权
 * @details 与 `Load::remove_activities` 不同, 这里不销毁 activity, 便于后续原位接回
 * @param node activity 所属 node
 * @param activity 待摘除的 activity
 * @return 被摘下的 activity (prev/next 已置空)
 */
Activity::UPtr splice_out_activity(Node* node, Activity* activity) {
  // 定位 activity 在 node 链表中的持有槽位: 头部是 node->first, 其余是前驱的 next
  Activity::UPtr* slot = (activity->prev == nullptr) ? &node->first : &activity->prev->next;
  Activity::UPtr detached = std::move(*slot);
  *slot = std::move(detached->next);
  if (*slot != nullptr) {
    (*slot)->prev = activity->prev;
  } else {
    node->last = activity->prev;
  }
  detached->prev = nullptr;
  detached->next = nullptr;
  return detached;
}

/**
 * @brief 将 activity 接回 node 链表中 `prev_activity` 之后
 * @param prev_activity 为 nullptr 表示插到链表头部
 */
void splice_in_activity_after(Node* node, Activity* prev_activity, Activity::UPtr activity) {
  activity->set_owner_node(node);
  activity->prev = prev_activity;
  Activity::UPtr* slot = (prev_activity == nullptr) ? &node->first : &prev_activity->next;
  activity->next = std::move(*slot);
  if (activity->next != nullptr) {
    activity->next->prev = activity.get();
  } else {
    node->last = activity.get();
  }
  *slot = std::move(activity);
}

}  // namespace

// ====== implement of select_worst_removal_indices ======
std::vector<std::size_t> select_worst_removal_indices(const std::vector<double>& gains,
                                                      std::size_t count, double noise_exponent,
                                                      RandomUtils& random) {
  // 收益降序; 收益相同时按下标升序, 保证同一种子下可复现
  std::vector<std::size_t> ranked(gains.size());
  std::iota(ranked.begin(), ranked.end(), std::size_t{0});
  std::ranges::sort(ranked, [&gains](const std::size_t lhs, const std::size_t rhs) {
    if (gains[lhs] != gains[rhs]) {
      return gains[lhs] > gains[rhs];
    }
    return lhs < rhs;
  });

  count = std::min(count, ranked.size());
  std::vector<std::size_t> selected;
  selected.reserve(count);
  for (std::size_t u = 0; u < count; ++u) {
    // y^p 越接近 0 越偏向收益最高者: p 越大越贪心, p == 1 时等价均匀随机
    const double y = std::pow(random.random_double(), noise_exponent);
    auto pos = static_cast<std::size_t>(y * static_cast<double>(ranked.size()));
    if (pos >= ranked.size()) {
      pos = ranked.size() - 1;
    }
    selected.push_back(ranked[pos]);
    ranked.erase(ranked.begin() + static_cast<std::ptrdiff_t>(pos));
  }
  return selected;
}

// ====== implement of AlnsRuinWorstRemovalOrder ======
AlnsRuinWorstRemovalOrder::AlnsRuinWorstRemovalOrder(
    const SolverContext* _context, const AlnsRuinWorstRemovalOrderParam& _parameter)
    : AlnsRuinOperator(AlnsRuinOperatorCode::RuinWorstRemovalOrder, _context),
      parameter(_parameter) {
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
                                ") must not exceed `select_order_max_rate` (" +
                                std::to_string(this->parameter.select_order_max_rate) + ")");
  }
  // p < 1 会把选择偏向收益最低者, 与"最差移除"语义相反
  if (this->parameter.select_order_noise_exponent < 1.0) {
    throw std::invalid_argument(this->code + ": `select_order_noise_exponent` must be >= 1, got " +
                                std::to_string(this->parameter.select_order_noise_exponent));
  }
}

std::string AlnsRuinWorstRemovalOrder::format_parameters() const {
  return std::format(
      "select_order_random_seed={}, select_order_min_rate={}, select_order_max_rate={}, "
      "select_order_noise_exponent={}",
      this->parameter.select_order_random_seed, this->parameter.select_order_min_rate,
      this->parameter.select_order_max_rate, this->parameter.select_order_noise_exponent);
}

std::unordered_map<int, AlnsRuinLoadActivity::UPtr> AlnsRuinWorstRemovalOrder::execute(
    Solution* sol) {
  std::unordered_map<int, AlnsRuinLoadActivity::UPtr> select_order_res;

  /**
   * @brief 候选订单及其移除收益
   */
  struct WorstCandidate {
    int load_ind;
    Activity* pick;
    double gain;
  };

  const int load_count = static_cast<int>(sol->loads.size());
  std::vector<WorstCandidate> candidates;

  for (int load_ind = 0; load_ind < load_count; ++load_ind) {
    Load* load = sol->loads[load_ind];
    std::vector<Activity*> pick_activities = load->get_pick_activities();
    std::size_t pick_activity_count = pick_activities.size();
    if (pick_activity_count == 0) {
      continue;
    }
    // 只有一个订单的 load: 移除即整 load 删除(pick node 与 drop node 一起摘空),
    // 收益就是该 load 的目标值; 多订单 load 的 pick node 移空由下面的按 node 摘除分支处理
    if (load->get_activity_count() <= 2) {
      const double obj_val = load->get_obj_val();
      if (obj_val >= LoadParameter::INIT_LOAD_OBJ_VAL) {
        throw std::runtime_error(this->code + ": load " + std::to_string(load_ind) +
                                 " has not been evaluated, obj_val = " + std::to_string(obj_val));
      }
      candidates.push_back(
          {.load_ind = load_ind, .pick = pick_activities.front(), .gain = obj_val});
      continue;
    }

    // 每个 load 只克隆一次: 在克隆体上反复"摘除-求值-接回", 原解不受影响
    std::unique_ptr<Load> tmp_load(this->context->problem->pd_pattern->deep_copy_load(load));
    const double baseline = tmp_load->get_obj_val();
    if (baseline >= LoadParameter::INIT_LOAD_OBJ_VAL) {
      throw std::runtime_error(this->code + ": load " + std::to_string(load_ind) +
                               " has not been evaluated, obj_val = " + std::to_string(baseline));
    }
    std::vector<Activity*> tmp_picks = tmp_load->get_pick_activities();
    if (tmp_picks.size() != pick_activity_count) {
      throw std::runtime_error(this->code + ": deep copy of load " + std::to_string(load_ind) +
                               " changed the order count");
    }
    for (std::size_t u = 0; u < pick_activity_count; ++u) {
      Activity* pick = tmp_picks[u];
      Activity* drop = pick->related;
      Node* pick_node = pick->owner_node;
      Node* drop_node = drop->owner_node;
      // SPMD 下提货集中在唯一的 pick node、送货按卸货点分 node, 两者同 node 说明不变量已被破坏
      if (pick_node == drop_node) {
        throw std::runtime_error(this->code + ": pick and drop of the same order share one node");
      }
      // 先记下还原锚点: 摘除会改写前驱/后继指针
      Activity* pick_prev = pick->prev;
      Activity* drop_prev = drop->prev;
      Node* pick_pred_node = pick_node->prev;
      Node* drop_pred_node = drop_node->prev;
      // node 只剩一个 activity 时, 摘完必须连 node 一起摘, 否则链上会留下空 node
      // (pick node 只在"该 load 仅剩此单"时移空, 但仍按同一规则处理, 不依赖提货集中这一前提)
      const bool pick_is_single_activity_node = pick_node->get_activity_count() == 1;
      const bool drop_is_single_activity_node = drop_node->get_activity_count() == 1;
      // 两个 node 相邻且都要整节点摘除: 必须一次摘下, 否则 drop 的锚点会随 pick node 脱离主链
      const bool adjacent_nodes = pick_is_single_activity_node && drop_is_single_activity_node &&
                                  drop_pred_node == pick_node;

      Activity::UPtr pick_activity_chain;
      Activity::UPtr drop_activity_chain;
      Node::UPtr pick_node_chain;
      Node::UPtr drop_node_chain;
      Node::UPtr adjacent_node_chain;
      if (adjacent_nodes) {
        adjacent_node_chain = NodeOps::splice_out(pick_node->prev->next, drop_node);
      } else {
        if (pick_is_single_activity_node) {
          pick_node_chain = NodeOps::splice_out(pick_node->prev->next, pick_node);
        } else {
          pick_activity_chain = splice_out_activity(pick_node, pick);
        }
        if (drop_is_single_activity_node) {
          drop_node_chain = NodeOps::splice_out(drop_node->prev->next, drop_node);
        } else {
          drop_activity_chain = splice_out_activity(drop_node, drop);
        }
      }

      tmp_load->reset_route_profile();
      this->context->problem->eval_load(tmp_load.get());
      const double gain = baseline - tmp_load->get_obj_val();
      const bool feasible = tmp_load->is_feasible();

      // 原位接回(不重算, 依赖下一轮的 eval_load 全量重算); 按摘除的逆序还原
      if (adjacent_nodes) {
        NodeOps::splice_in_after(pick_pred_node, std::move(adjacent_node_chain));
      } else {
        if (drop_is_single_activity_node) {
          NodeOps::splice_in_after(drop_pred_node, std::move(drop_node_chain));
        } else {
          splice_in_activity_after(drop_node, drop_prev, std::move(drop_activity_chain));
        }
        if (pick_is_single_activity_node) {
          NodeOps::splice_in_after(pick_pred_node, std::move(pick_node_chain));
        } else {
          splice_in_activity_after(pick_node, pick_prev, std::move(pick_activity_chain));
        }
      }

      if (feasible) {
        candidates.push_back({.load_ind = load_ind, .pick = pick_activities[u], .gain = gain});
      }
    }
  }

  const int order_count = static_cast<int>(candidates.size());
  if (order_count == 0) {
    return select_order_res;
  }

  std::vector<double> gains;
  gains.reserve(candidates.size());
  for (const auto& c : candidates) {
    gains.push_back(c.gain);
  }

  // 移除数量按已分配订单总数取比例, 再截断到有效候选数
  const int select_count = std::max(
      1, this->select_order_random->random_int(order_count, this->parameter.select_order_min_rate,
                                               this->parameter.select_order_max_rate));
  const std::vector<std::size_t> selected = select_worst_removal_indices(
      gains, select_count, this->parameter.select_order_noise_exponent, *this->select_order_random);

  for (const std::size_t local_ind : selected) {
    const WorstCandidate& candidate = candidates[local_ind];
    auto& load_activities = select_order_res[candidate.load_ind];
    if (load_activities == nullptr) {
      load_activities = std::make_unique<AlnsRuinLoadActivity>(candidate.load_ind);
    }
    // 提卸成对移除
    load_activities->activities.emplace_back(candidate.pick);
    load_activities->activities.emplace_back(candidate.pick->related);
  }

  return select_order_res;
}
