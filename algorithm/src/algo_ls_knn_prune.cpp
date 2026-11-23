/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#include "algo_ls_knn_prune.h"

#include <cstddef>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

#include "algo_ls_neighborhood.h"
#include "pdm_solution.h"

namespace {

/**
 * @brief 指向某个 Load 的第 unit_ind 个可移动单元
 */
struct UnitRef {
  int load_ind{-1};
  int unit_ind{-1};
};

/**
 * @brief 一对新建的相邻单元
 */
struct Adjacency {
  UnitRef before;
  UnitRef after;
};

/**
 * @brief 一个 move 最多产生 4 对新建相邻关系（exchange 的左右各两处）
 */
class AdjacencyList {
 public:
  void add(const int before_load, const int before_unit, const int after_load,
           const int after_unit) {
    if (before_load < 0 || before_unit < 0 || after_load < 0 || after_unit < 0) {
      return;
    }
    if (this->size_ >= kCapacity) {
      return;
    }
    this->items_[this->size_] = Adjacency{.before = UnitRef{before_load, before_unit},
                                          .after = UnitRef{after_load, after_unit}};
    ++this->size_;
  }

  [[nodiscard]] int size() const { return this->size_; }

  [[nodiscard]] const Adjacency& operator[](const int index) const { return this->items_[index]; }

 private:
  static constexpr int kCapacity = 4;

  Adjacency items_[kCapacity];
  int size_{0};
};

/**
 * @brief 两个相邻单元的订单之间是否存在 KNN 边
 */
[[nodiscard]] bool units_adjacent(const KnnGraph& graph, const LsKnnPrune::UnitOrderTable& table,
                                  const UnitRef& before, const UnitRef& after) {
  const std::vector<int>* before_orders = table.unit_orders(before.load_ind, before.unit_ind);
  const std::vector<int>* after_orders = table.unit_orders(after.load_ind, after.unit_ind);
  if (before_orders == nullptr || after_orders == nullptr) {
    return false;
  }
  for (const int before_order : *before_orders) {
    for (const int after_order : *after_orders) {
      if (LsKnnPrune::is_knn_edge(graph, before_order, after_order)) {
        return true;
      }
    }
  }
  return false;
}

void collect_relocate_adjacencies(const LsRelocateMove& relocate,
                                  const LsKnnPrune::UnitOrderTable& table, AdjacencyList* out) {
  const LsBlock& block = relocate.block;
  const int block_last = block.first_unit + block.unit_count - 1;
  if (relocate.to_load == block.load_ind) {
    // 同车：槽位按移除源块后的剩余序列计，映射回移除前下标
    const int remaining = table.unit_count(block.load_ind) - block.unit_count;
    const auto map_slot = [&block](const int slot) {
      return slot < block.first_unit ? slot : slot + block.unit_count;
    };
    if (relocate.to_slot > 0) {
      out->add(block.load_ind, map_slot(relocate.to_slot - 1), block.load_ind, block.first_unit);
    }
    if (relocate.to_slot < remaining) {
      out->add(block.load_ind, block_last, block.load_ind, map_slot(relocate.to_slot));
    }
    return;
  }
  if (relocate.to_slot > 0) {
    out->add(relocate.to_load, relocate.to_slot - 1, block.load_ind, block.first_unit);
  }
  if (relocate.to_slot < table.unit_count(relocate.to_load)) {
    out->add(block.load_ind, block_last, relocate.to_load, relocate.to_slot);
  }
}

void collect_exchange_adjacencies(const LsExchangeMove& exchange,
                                  const LsKnnPrune::UnitOrderTable& table, AdjacencyList* out) {
  const LsBlock& first = exchange.first;
  const LsBlock& second = exchange.second;
  const int first_last = first.first_unit + first.unit_count - 1;
  const int second_last = second.first_unit + second.unit_count - 1;
  const int first_after = first.first_unit + first.unit_count;
  const int second_after = second.first_unit + second.unit_count;
  if (first.load_ind == second.load_ind && second.first_unit == first_after) {
    // 同车两块紧邻：互换后仍相邻，彼此成为新邻居，左右邻接按整体窗口算
    if (first.first_unit > 0) {
      out->add(first.load_ind, first.first_unit - 1, second.load_ind, second.first_unit);
    }
    out->add(second.load_ind, second_last, first.load_ind, first.first_unit);
    if (second_after < table.unit_count(first.load_ind)) {
      out->add(first.load_ind, first_last, second.load_ind, second_after);
    }
    return;
  }
  // 两个块互换位置后各自的左邻居
  if (first.first_unit > 0) {
    out->add(first.load_ind, first.first_unit - 1, second.load_ind, second.first_unit);
  }
  if (second.first_unit > 0) {
    out->add(second.load_ind, second.first_unit - 1, first.load_ind, first.first_unit);
  }
  // 两个块互换位置后各自的右邻居
  if (first_after < table.unit_count(first.load_ind)) {
    out->add(second.load_ind, second_last, first.load_ind, first_after);
  }
  if (second_after < table.unit_count(second.load_ind)) {
    out->add(first.load_ind, first_last, second.load_ind, second_after);
  }
}

void collect_reverse_adjacencies(const LsReverseMove& reverse,
                                 const LsKnnPrune::UnitOrderTable& table, AdjacencyList* out) {
  const LsBlock& block = reverse.block;
  const int span_last = block.first_unit + block.unit_count - 1;
  const int span_after = block.first_unit + block.unit_count;
  // 反转后段尾接到段前，段首接到段后
  if (block.first_unit > 0) {
    out->add(block.load_ind, block.first_unit - 1, block.load_ind, span_last);
  }
  if (span_after < table.unit_count(block.load_ind)) {
    out->add(block.load_ind, block.first_unit, block.load_ind, span_after);
  }
}

}  // namespace

// ====== implement of LsKnnPrune::UnitOrderTable ======
namespace LsKnnPrune {

UnitOrderTable::UnitOrderTable(std::vector<std::vector<std::vector<int>>> units)
    : units_(std::move(units)) {}

UnitOrderTable UnitOrderTable::from_solution(const Solution& sol,
                                             const LsNeighborhood& neighborhood) {
  UnitOrderTable table;
  table.units_.reserve(sol.loads.size());
  for (int load_ind = 0; load_ind < static_cast<int>(sol.loads.size()); ++load_ind) {
    table.units_.push_back(neighborhood.load_unit_orders(sol, load_ind));
  }
  return table;
}

int UnitOrderTable::unit_count(const int load_ind) const {
  if (load_ind < 0 || load_ind >= static_cast<int>(this->units_.size())) {
    return 0;
  }
  return static_cast<int>(this->units_[static_cast<std::size_t>(load_ind)].size());
}

const std::vector<int>* UnitOrderTable::unit_orders(const int load_ind, const int unit_ind) const {
  if (unit_ind < 0 || unit_ind >= this->unit_count(load_ind)) {
    return nullptr;
  }
  return &this->units_[static_cast<std::size_t>(load_ind)][static_cast<std::size_t>(unit_ind)];
}

bool is_knn_edge(const KnnGraph& graph, const int from_order, const int to_order) {
  if (from_order < 0 || to_order < 0 || from_order == to_order) {
    return false;
  }
  const auto from_row = graph.find(from_order);
  if (from_row != graph.end() && from_row->second.contains(to_order)) {
    return true;
  }
  const auto to_row = graph.find(to_order);
  return to_row != graph.end() && to_row->second.contains(from_order);
}

bool passes(const LsMove& move, const UnitOrderTable& table, const KnnGraph& graph) {
  AdjacencyList adjacencies;
  std::visit(
      [&table, &adjacencies](const auto& typed_move) {
        using MoveType = std::decay_t<decltype(typed_move)>;
        if constexpr (std::is_same_v<MoveType, LsRelocateMove>) {
          collect_relocate_adjacencies(typed_move, table, &adjacencies);
        } else if constexpr (std::is_same_v<MoveType, LsExchangeMove>) {
          collect_exchange_adjacencies(typed_move, table, &adjacencies);
        } else if constexpr (std::is_same_v<MoveType, LsReverseMove>) {
          collect_reverse_adjacencies(typed_move, table, &adjacencies);
        }
      },
      move);
  if (adjacencies.size() == 0) {
    return true;
  }
  for (int u = 0; u < adjacencies.size(); ++u) {
    const Adjacency& adjacency = adjacencies[u];
    if (units_adjacent(graph, table, adjacency.before, adjacency.after)) {
      return true;
    }
  }
  return false;
}

}  // namespace LsKnnPrune
