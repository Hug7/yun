/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#include "algo_ls_dist_prune.h"

#include <algorithm>
#include <cstddef>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

#include "algo_ls_node_edit.h"
#include "bdm_dist_matrix.h"
#include "pdm_load.h"
#include "pdm_node.h"
#include "pdm_solution.h"

namespace LsDistPrune {

DistanceTable::DistanceTable(std::vector<RouteSnapshot> routes) : routes_(std::move(routes)) {}

DistanceTable DistanceTable::from_solution(const Solution& sol) {
  std::vector<RouteSnapshot> routes;
  routes.reserve(sol.loads.size());
  for (const Load* load : sol.loads) {
    RouteSnapshot route;
    if (load != nullptr && load->vehicle != nullptr) {
      route.dist_matrix = load->vehicle->get_dist_matrix();
      route.feasible = load->is_feasible();
      if (const Node* pick = LsNodeEdit::find_pick_node(load->first_node); pick != nullptr) {
        route.pick_loc_ind = pick->loc != nullptr ? pick->loc->ind : -1;
        for (const Node* node = pick->next.get(); node != nullptr && node->is_drop();
             node = node->next.get()) {
          route.unit_loc_inds.push_back(node->loc != nullptr ? node->loc->ind : -1);
        }
      }
      if (load->last_node != nullptr && load->last_node->loc != nullptr) {
        route.end_loc_ind = load->last_node->loc->ind;
      }
    }
    routes.push_back(std::move(route));
  }
  return DistanceTable(std::move(routes));
}

const RouteSnapshot* DistanceTable::route(const int load_ind) const {
  if (load_ind < 0 || load_ind >= static_cast<int>(this->routes_.size())) {
    return nullptr;
  }
  return &this->routes_[static_cast<std::size_t>(load_ind)];
}

namespace {

/**
 * @brief 序列第 i 条弧：0 是 PICK 出发，unit_count 是回到 END，其余连前一个单元
 */
bool arc_dist(const RouteSnapshot& route, const std::vector<int>& units, const std::size_t i,
              std::int64_t* out) {
  const std::size_t unit_count = units.size();
  if (route.dist_matrix == nullptr || i > unit_count) {
    return false;
  }
  const int from_loc = i == 0 ? route.pick_loc_ind : units[i - 1];
  const int to_loc = i == unit_count ? route.end_loc_ind : units[i];
  if (from_loc < 0 || to_loc < 0) {
    return false;
  }
  *out = route.dist_matrix->get_dist(from_loc, to_loc);
  return true;
}

/**
 * @brief 新序列相对旧序列的里程变化，从首个不同位置开始重算
 */
std::optional<std::int64_t> sequence_delta(const RouteSnapshot& route,
                                           const std::vector<int>& new_units) {
  const std::vector<int>& old_units = route.unit_loc_inds;
  const std::size_t old_count = old_units.size();
  const std::size_t new_count = new_units.size();
  std::size_t common = 0;
  while (common < old_count && common < new_count && old_units[common] == new_units[common]) {
    ++common;
  }
  if (common == old_count && common == new_count) {
    return 0;
  }
  std::int64_t delta = 0;
  for (std::size_t i = common; i <= new_count; ++i) {
    std::int64_t dist = 0;
    if (!arc_dist(route, new_units, i, &dist)) {
      return std::nullopt;
    }
    delta += dist;
  }
  for (std::size_t i = common; i <= old_count; ++i) {
    std::int64_t dist = 0;
    if (!arc_dist(route, old_units, i, &dist)) {
      return std::nullopt;
    }
    delta -= dist;
  }
  return delta;
}

bool is_valid_block(const LsBlock& block, const int unit_count) {
  return block.load_ind >= 0 && block.first_unit >= 0 && block.unit_count >= 1 &&
         block.first_unit + block.unit_count <= unit_count;
}

std::vector<int> block_loc_inds(const std::vector<int>& units, const LsBlock& block) {
  const auto first = units.begin() + block.first_unit;
  return std::vector<int>(first, first + block.unit_count);
}

void erase_block(const std::vector<int>& units, const LsBlock& block, std::vector<int>* out) {
  out->clear();
  out->reserve(units.size() - static_cast<std::size_t>(block.unit_count));
  out->insert(out->end(), units.begin(), units.begin() + block.first_unit);
  out->insert(out->end(), units.begin() + block.first_unit + block.unit_count, units.end());
}

void insert_block(const std::vector<int>& units, const std::vector<int>& loc_inds, const int slot,
                  std::vector<int>* out) {
  out->clear();
  out->reserve(units.size() + loc_inds.size());
  out->insert(out->end(), units.begin(), units.begin() + slot);
  out->insert(out->end(), loc_inds.begin(), loc_inds.end());
  out->insert(out->end(), units.begin() + slot, units.end());
}

/**
 * @brief 用 loc_inds 原地替换 units 中的 block 段
 */
void replace_block(const std::vector<int>& units, const LsBlock& block,
                   const std::vector<int>& loc_inds, std::vector<int>* out) {
  out->clear();
  out->reserve(units.size() - static_cast<std::size_t>(block.unit_count) + loc_inds.size());
  out->insert(out->end(), units.begin(), units.begin() + block.first_unit);
  out->insert(out->end(), loc_inds.begin(), loc_inds.end());
  out->insert(out->end(), units.begin() + block.first_unit + block.unit_count, units.end());
}

/**
 * @brief 左块与右块互换位置：右块占据左块原位，左块跟在右块之后
 */
void swap_blocks(const std::vector<int>& units, const LsBlock& left, const LsBlock& right,
                 std::vector<int>* out) {
  const int left_begin = left.first_unit;
  const int left_end = left.first_unit + left.unit_count;
  const int right_begin = right.first_unit;
  const int right_end = right.first_unit + right.unit_count;
  out->clear();
  out->reserve(units.size());
  out->insert(out->end(), units.begin(), units.begin() + left_begin);
  out->insert(out->end(), units.begin() + right_begin, units.begin() + right_end);
  out->insert(out->end(), units.begin() + left_end, units.begin() + right_begin);
  out->insert(out->end(), units.begin() + left_begin, units.begin() + left_end);
  out->insert(out->end(), units.begin() + right_end, units.end());
}

std::optional<std::int64_t> relocate_delta(const LsRelocateMove& move, const DistanceTable& table) {
  const RouteSnapshot* from = table.route(move.block.load_ind);
  const RouteSnapshot* to = table.route(move.to_load);
  if (from == nullptr || to == nullptr || !is_valid_block(move.block, from->unit_count())) {
    return std::nullopt;
  }
  const std::vector<int> loc_inds = block_loc_inds(from->unit_loc_inds, move.block);
  if (from == to) {
    const int remaining = from->unit_count() - move.block.unit_count;
    if (move.to_slot < 0 || move.to_slot > remaining) {
      return std::nullopt;
    }
    std::vector<int> erased;
    erase_block(from->unit_loc_inds, move.block, &erased);
    std::vector<int> new_units;
    insert_block(erased, loc_inds, move.to_slot, &new_units);
    return sequence_delta(*from, new_units);
  }
  if (move.to_slot < 0 || move.to_slot > to->unit_count()) {
    return std::nullopt;
  }
  std::vector<int> from_units;
  erase_block(from->unit_loc_inds, move.block, &from_units);
  std::vector<int> to_units;
  insert_block(to->unit_loc_inds, loc_inds, move.to_slot, &to_units);
  const std::optional<std::int64_t> from_delta = sequence_delta(*from, from_units);
  const std::optional<std::int64_t> to_delta = sequence_delta(*to, to_units);
  if (!from_delta.has_value() || !to_delta.has_value()) {
    return std::nullopt;
  }
  return *from_delta + *to_delta;
}

std::optional<std::int64_t> exchange_delta(const LsExchangeMove& move, const DistanceTable& table) {
  const RouteSnapshot* first_route = table.route(move.first.load_ind);
  const RouteSnapshot* second_route = table.route(move.second.load_ind);
  if (first_route == nullptr || second_route == nullptr ||
      !is_valid_block(move.first, first_route->unit_count()) ||
      !is_valid_block(move.second, second_route->unit_count())) {
    return std::nullopt;
  }
  const std::vector<int> first_locs = block_loc_inds(first_route->unit_loc_inds, move.first);
  if (first_route == second_route) {
    if (move.first.first_unit == move.second.first_unit) {
      return std::nullopt;
    }
    const LsBlock& left = move.first.first_unit < move.second.first_unit ? move.first : move.second;
    const LsBlock& right =
        move.first.first_unit < move.second.first_unit ? move.second : move.first;
    if (left.first_unit + left.unit_count > right.first_unit) {
      return std::nullopt;
    }
    std::vector<int> new_units;
    swap_blocks(first_route->unit_loc_inds, left, right, &new_units);
    return sequence_delta(*first_route, new_units);
  }
  const std::vector<int> second_locs = block_loc_inds(second_route->unit_loc_inds, move.second);
  std::vector<int> first_units;
  replace_block(first_route->unit_loc_inds, move.first, second_locs, &first_units);
  std::vector<int> second_units;
  replace_block(second_route->unit_loc_inds, move.second, first_locs, &second_units);
  const std::optional<std::int64_t> first_delta = sequence_delta(*first_route, first_units);
  const std::optional<std::int64_t> second_delta = sequence_delta(*second_route, second_units);
  if (!first_delta.has_value() || !second_delta.has_value()) {
    return std::nullopt;
  }
  return *first_delta + *second_delta;
}

std::optional<std::int64_t> reverse_delta(const LsReverseMove& move, const DistanceTable& table) {
  const RouteSnapshot* route = table.route(move.block.load_ind);
  if (route == nullptr || !is_valid_block(move.block, route->unit_count())) {
    return std::nullopt;
  }
  std::vector<int> new_units = route->unit_loc_inds;
  std::reverse(new_units.begin() + move.block.first_unit,
               new_units.begin() + move.block.first_unit + move.block.unit_count);
  return sequence_delta(*route, new_units);
}

bool load_infeasible(const DistanceTable& table, const int load_ind) {
  const RouteSnapshot* route = table.route(load_ind);
  return route != nullptr && !route->feasible;
}

bool any_load_infeasible(const LsMove& move, const DistanceTable& table) {
  return std::visit(
      [&table](const auto& typed_move) {
        using MoveType = std::decay_t<decltype(typed_move)>;
        if constexpr (std::is_same_v<MoveType, LsRelocateMove>) {
          return load_infeasible(table, typed_move.block.load_ind) ||
                 load_infeasible(table, typed_move.to_load);
        } else if constexpr (std::is_same_v<MoveType, LsExchangeMove>) {
          return load_infeasible(table, typed_move.first.load_ind) ||
                 load_infeasible(table, typed_move.second.load_ind);
        } else {
          return load_infeasible(table, typed_move.block.load_ind);
        }
      },
      move);
}

}  // namespace

std::optional<std::int64_t> distance_delta(const LsMove& move, const DistanceTable& table) {
  return std::visit(
      [&table](const auto& typed_move) {
        using MoveType = std::decay_t<decltype(typed_move)>;
        if constexpr (std::is_same_v<MoveType, LsRelocateMove>) {
          return relocate_delta(typed_move, table);
        } else if constexpr (std::is_same_v<MoveType, LsExchangeMove>) {
          return exchange_delta(typed_move, table);
        } else {
          return reverse_delta(typed_move, table);
        }
      },
      move);
}

bool passes(const LsMove& move, const DistanceTable& table) {
  if (table.empty() || any_load_infeasible(move, table)) {
    return true;
  }
  const std::optional<std::int64_t> delta = distance_delta(move, table);
  if (!delta.has_value()) {
    return true;
  }
  return *delta <= 0;
}

}  // namespace LsDistPrune
