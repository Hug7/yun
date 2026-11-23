/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#include "algo_ls_neighborhood_spmd.h"

#include <cstddef>
#include <memory>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

#include "algo_ls_move.h"
#include "algo_ls_node_edit.h"
#include "c_constant.h"
#include "pdm_activity.h"
#include "pdm_context.h"
#include "pdm_load.h"
#include "pdm_order.h"
#include "pdm_solution.h"
#include "prob_pattern.h"

namespace {

const Load* load_at(const Solution& sol, const int load_ind) {
  if (load_ind < 0 || load_ind >= static_cast<int>(sol.loads.size())) {
    return nullptr;
  }
  return sol.loads[static_cast<std::size_t>(load_ind)];
}

}  // namespace

// ====== implement of SpmdNeighborhood：结构编辑与装配 ======
SpmdNeighborhood::SpmdNeighborhood(PickDropPattern* pattern) : pattern_(pattern) {}

int SpmdNeighborhood::movable_unit_count(const Solution& sol, const int load_ind) const {
  const Load* load = load_at(sol, load_ind);
  if (load == nullptr) {
    return 0;
  }

  // 直接从链上数 drop 节点，不依赖 LoadRouteProfile 的缓存，因此无需先 reset
  return LsNodeEdit::drop_node_count(load->first_node);
}

std::vector<std::vector<int>> SpmdNeighborhood::load_unit_orders(const Solution& sol,
                                                                 const int load_ind) const {
  const Load* load = load_at(sol, load_ind);
  if (load == nullptr) {
    return {};
  }
  const std::vector<Node*> drops = LsNodeEdit::collect_drop_nodes(load->first_node);
  std::vector<std::vector<int>> units;
  units.reserve(drops.size());
  for (const Node* drop : drops) {
    std::vector<int> order_indices;
    for (const Activity* activity = drop->first.get(); activity != nullptr;
         activity = activity->next.get()) {
      if (activity->order != nullptr) {
        order_indices.push_back(activity->order->ind);
      }
    }
    units.push_back(std::move(order_indices));
  }
  return units;
}

std::vector<int> SpmdNeighborhood::candidate_target_loads(const Solution& sol,
                                                          const LsBlock& moved) const {
  const Load* from = load_at(sol, moved.load_ind);
  if (from == nullptr || !LsNodeEdit::is_valid_block(from->first_node, moved)) {
    return {};
  }
  const Node* from_pick = LsNodeEdit::find_pick_node(from->first_node);
  if (from_pick == nullptr) {
    return {};
  }
  std::vector<int> targets;
  targets.reserve(sol.loads.size());
  for (int load_ind = 0; load_ind < static_cast<int>(sol.loads.size()); ++load_ind) {
    if (load_ind == moved.load_ind) {
      continue;
    }
    const Load* load = sol.loads[static_cast<std::size_t>(load_ind)];
    if (load == nullptr) {
      continue;
    }
    // SPMD 跨车前提：两车共享同一提货站点
    const Node* pick = LsNodeEdit::find_pick_node(load->first_node);
    if (pick == nullptr || pick->loc != from_pick->loc) {
      continue;
    }
    targets.push_back(load_ind);
  }
  return targets;
}

std::vector<int> SpmdNeighborhood::target_slots(const Solution& sol, const LsBlock& moved,
                                                const int to_load) const {
  const Load* from = load_at(sol, moved.load_ind);
  const Load* to = load_at(sol, to_load);
  if (from == nullptr || to == nullptr || !LsNodeEdit::is_valid_block(from->first_node, moved)) {
    return {};
  }
  const Node* from_pick = LsNodeEdit::find_pick_node(from->first_node);
  const Node* to_pick = LsNodeEdit::find_pick_node(to->first_node);
  if (from_pick == nullptr || to_pick == nullptr) {
    return {};
  }
  const bool intra = to_load == moved.load_ind;
  if (!intra && to_pick->loc != from_pick->loc) {
    return {};
  }
  const int to_unit_count = LsNodeEdit::drop_node_count(to->first_node);
  // 同车槽位按移除源块后的剩余序列计
  const int slot_count = intra ? to_unit_count - moved.unit_count : to_unit_count;
  std::vector<int> slots;
  slots.reserve(static_cast<std::size_t>(slot_count) + 1);
  for (int slot = 0; slot <= slot_count; ++slot) {
    slots.push_back(slot);
  }
  return slots;
}

Load* SpmdNeighborhood::copy_load(const Load& load) const {
  if (pattern_ == nullptr) {
    return nullptr;
  }
  return pattern_->deep_copy_load(&load);
}

bool SpmdNeighborhood::apply(Load* from, Load* to, const LsMove& move) const {
  if (from == nullptr || to == nullptr) {
    return false;
  }
  return std::visit(
      [from, to](const auto& concrete_move) -> bool {
        using MoveType = std::decay_t<decltype(concrete_move)>;
        if constexpr (std::is_same_v<MoveType, LsRelocateMove>) {
          if (from == to) {
            return LsNodeEdit::relocate_intra(from->first_node, concrete_move.block,
                                              concrete_move.to_slot);
          }
          return LsNodeEdit::relocate_cross(from->first_node, to->first_node, concrete_move.block,
                                            concrete_move.to_slot);
        } else if constexpr (std::is_same_v<MoveType, LsExchangeMove>) {
          if (from == to) {
            return LsNodeEdit::exchange_intra(from->first_node, concrete_move.first,
                                              concrete_move.second);
          }
          return LsNodeEdit::exchange_cross(from->first_node, to->first_node, concrete_move.first,
                                            concrete_move.second);
        } else {
          // 2-opt 只在同车内做
          if (from != to) {
            return false;
          }
          return LsNodeEdit::reverse(from->first_node, concrete_move.block);
        }
      },
      move);
}

void SpmdNeighborhood::canonicalize(Load* load) const {
  if (load == nullptr) {
    return;
  }
  LsNodeEdit::canonicalize(load->first_node);
}

// ====== implement of factory ======
std::unique_ptr<LsNeighborhood> create_ls_neighborhood(const SolverContext* context) {
  if (context->parameter->pick_drop_pattern != PickDropPatternType::SPMD) {
    return nullptr;
  }
  if (context->problem->pd_pattern == nullptr) {
    return nullptr;
  }
  return std::make_unique<SpmdNeighborhood>(context->problem->pd_pattern);
}
