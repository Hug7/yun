/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#include "algo_ls_node_edit.h"

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <utility>
#include <vector>

namespace {

/**
 * @brief 摘出 drops[first_unit, first_unit + unit_count) 对应的 DROP 子链
 * @details 约定 first_unit == 0 时首块的前驱就是 PICK 节点，因此 prev 一定非空。
 */
Node::UPtr take_block(const std::vector<Node*>& drops, const int first_unit, const int unit_count) {
  assert(unit_count >= 1);
  Node::UPtr& from_ptr = drops[static_cast<std::size_t>(first_unit)]->prev->next;
  Node* block_last = drops[static_cast<std::size_t>(first_unit + unit_count - 1)];
  return NodeOps::splice_out(from_ptr, block_last);
}

/**
 * @brief 取块覆盖的 DROP 节点列表（与 drops 同序）
 */
std::vector<Node*> slice_block(const std::vector<Node*>& drops, const int first_unit,
                               const int unit_count) {
  std::vector<Node*> nodes;
  nodes.reserve(static_cast<std::size_t>(unit_count));
  for (int ind = first_unit; ind < first_unit + unit_count; ++ind) {
    nodes.push_back(drops[static_cast<std::size_t>(ind)]);
  }
  return nodes;
}

/**
 * @brief 校验块内每个 drop activity 都能找到源 PICK 节点上的配对 pick activity
 */
bool can_move_pick_activities(const Node* from_pick, const std::vector<Node*>& drop_nodes) {
  for (const Node* node : drop_nodes) {
    for (const Activity* activity = node->first.get(); activity != nullptr;
         activity = activity->next.get()) {
      const Activity* related = activity->related;
      if (related == nullptr || related->activity_type != ActivityType::PICK) {
        return false;
      }
      if (related->owner_node != from_pick) {
        return false;
      }
      if (related->order != activity->order) {
        return false;
      }
    }
  }
  return true;
}

/**
 * @brief 把 drop_nodes 上全部订单的 pick activity 从 from_pick 搬到 to_pick
 * @pre can_move_pick_activities(from_pick, drop_nodes)
 */
void move_pick_activities(Node* from_pick, Node* to_pick, const std::vector<Node*>& drop_nodes) {
  for (Node* node : drop_nodes) {
    for (const Activity* activity = node->first.get(); activity != nullptr;
         activity = activity->next.get()) {
      Activity::UPtr pick_activity = LsNodeEdit::detach_activity(from_pick, activity->related);
      if (pick_activity == nullptr) {
        continue;
      }
      to_pick->add_back_activity(std::move(pick_activity));
    }
  }
}

}  // namespace

// ====== implement of LsNodeEdit ======
namespace LsNodeEdit {

Node* find_pick_node(const Node::UPtr& first_node) {
  if (!first_node || !first_node->next) {
    return nullptr;
  }
  Node* node = first_node->next.get();
  return node->is_pick() ? node : nullptr;
}

int drop_node_count(const Node::UPtr& first_node) {
  Node* pick_node = find_pick_node(first_node);
  if (pick_node == nullptr) {
    return 0;
  }
  int count = 0;
  for (Node* node = pick_node->next.get(); node != nullptr && node->is_drop();
       node = node->next.get()) {
    ++count;
  }
  return count;
}

std::vector<Node*> collect_drop_nodes(const Node::UPtr& first_node) {
  std::vector<Node*> drops;
  Node* pick_node = find_pick_node(first_node);
  if (pick_node == nullptr) {
    return drops;
  }
  for (Node* node = pick_node->next.get(); node != nullptr && node->is_drop();
       node = node->next.get()) {
    drops.push_back(node);
  }
  return drops;
}

bool is_valid_block(const Node::UPtr& first_node, const LsBlock& block) {
  if (block.unit_count < 1 || block.first_unit < 0) {
    return false;
  }
  return block.first_unit + block.unit_count <= drop_node_count(first_node);
}

Node* slot_anchor(const Node::UPtr& first_node, const int slot) {
  if (slot < 0) {
    return nullptr;
  }
  Node* pick_node = find_pick_node(first_node);
  if (pick_node == nullptr) {
    return nullptr;
  }
  if (slot == 0) {
    return pick_node;
  }
  int unit_ind = 0;
  for (Node* node = pick_node->next.get(); node != nullptr && node->is_drop();
       node = node->next.get()) {
    ++unit_ind;
    if (unit_ind == slot) {
      return node;
    }
  }
  return nullptr;
}

Activity::UPtr detach_activity(Node* node, Activity* activity) {
  if (node == nullptr || activity == nullptr || activity->owner_node != node) {
    return nullptr;
  }
  Activity::UPtr owned;
  if (activity->prev == nullptr) {
    // 首 activity：所有权从 node->first 转移给 owned，node 接上其后继
    owned = std::move(node->first);
    node->first = std::move(owned->next);
    if (node->first != nullptr) {
      node->first->prev = nullptr;
    } else {
      node->last = nullptr;
    }
  } else {
    Activity* prev_activity = activity->prev;
    owned = std::move(prev_activity->next);
    prev_activity->next = std::move(owned->next);
    if (prev_activity->next != nullptr) {
      prev_activity->next->prev = prev_activity;
    } else {
      node->last = prev_activity;
    }
  }
  owned->prev = nullptr;
  owned->next = nullptr;
  owned->owner_node = nullptr;
  return owned;
}

bool relocate_intra(Node::UPtr& first_node, const LsBlock& block, const int to_slot) {
  if (!is_valid_block(first_node, block)) {
    return false;
  }
  const int unit_count = drop_node_count(first_node);
  const int remaining = unit_count - block.unit_count;
  if (to_slot < 0 || to_slot > remaining) {
    return false;
  }
  const std::vector<Node*> drops = collect_drop_nodes(first_node);
  // 槽位按移除后的序列计，映射回移除前的下标后再取锚点（锚点必然落在块外）
  const int mapped_slot = to_slot <= block.first_unit ? to_slot : to_slot + block.unit_count;
  Node* anchor = slot_anchor(first_node, mapped_slot);
  if (anchor == nullptr) {
    return false;
  }
  Node::UPtr chain = take_block(drops, block.first_unit, block.unit_count);
  NodeOps::splice_in_after(anchor, std::move(chain));
  return true;
}

bool relocate_cross(Node::UPtr& from_first_node, Node::UPtr& to_first_node, const LsBlock& block,
                    const int to_slot) {
  Node* from_pick = find_pick_node(from_first_node);
  Node* to_pick = find_pick_node(to_first_node);
  if (from_pick == nullptr || to_pick == nullptr || from_pick->loc != to_pick->loc) {
    return false;
  }
  if (!is_valid_block(from_first_node, block)) {
    return false;
  }
  const int from_unit_count = drop_node_count(from_first_node);
  if (block.unit_count >= from_unit_count) {
    // v1 禁止把车次搬空
    return false;
  }
  const int to_unit_count = drop_node_count(to_first_node);
  if (to_slot < 0 || to_slot > to_unit_count) {
    return false;
  }
  const std::vector<Node*> drops = collect_drop_nodes(from_first_node);
  const std::vector<Node*> block_nodes = slice_block(drops, block.first_unit, block.unit_count);
  if (!can_move_pick_activities(from_pick, block_nodes)) {
    return false;
  }
  Node* anchor = slot_anchor(to_first_node, to_slot);
  if (anchor == nullptr) {
    return false;
  }
  move_pick_activities(from_pick, to_pick, block_nodes);
  Node::UPtr chain = take_block(drops, block.first_unit, block.unit_count);
  NodeOps::splice_in_after(anchor, std::move(chain));
  return true;
}

bool exchange_intra(Node::UPtr& first_node, const LsBlock& first, const LsBlock& second) {
  if (!is_valid_block(first_node, first) || !is_valid_block(first_node, second)) {
    return false;
  }
  if (first.first_unit == second.first_unit) {
    return false;
  }
  const LsBlock& left = first.first_unit < second.first_unit ? first : second;
  const LsBlock& right = first.first_unit < second.first_unit ? second : first;
  const bool adjacent = left.first_unit + left.unit_count == right.first_unit;
  if (!adjacent && left.first_unit + left.unit_count > right.first_unit) {
    return false;
  }
  const std::vector<Node*> drops = collect_drop_nodes(first_node);
  Node* left_first = drops[static_cast<std::size_t>(left.first_unit)];
  Node* right_first = drops[static_cast<std::size_t>(right.first_unit)];
  Node* right_last = drops[static_cast<std::size_t>(right.first_unit + right.unit_count - 1)];
  // 预先记住两处锚点：左块原位的前驱、右块原位的前驱（右块之前最后一个中间单元）
  Node* anchor_left = left_first->prev;
  Node* mid_last = right_first->prev;
  Node::UPtr right_chain = take_block(drops, right.first_unit, right.unit_count);
  Node::UPtr left_chain = take_block(drops, left.first_unit, left.unit_count);
  // 右块占左块原位；左块插到右块原位（相邻时直接跟在右块之后）
  NodeOps::splice_in_after(anchor_left, std::move(right_chain));
  Node* anchor_left_after = adjacent ? right_last : mid_last;
  NodeOps::splice_in_after(anchor_left_after, std::move(left_chain));
  return true;
}

bool exchange_cross(Node::UPtr& from_first_node, Node::UPtr& to_first_node,
                    const LsBlock& from_block, const LsBlock& to_block) {
  Node* from_pick = find_pick_node(from_first_node);
  Node* to_pick = find_pick_node(to_first_node);
  if (from_pick == nullptr || to_pick == nullptr || from_pick->loc != to_pick->loc) {
    return false;
  }
  if (!is_valid_block(from_first_node, from_block) || !is_valid_block(to_first_node, to_block)) {
    return false;
  }
  const std::vector<Node*> from_drops = collect_drop_nodes(from_first_node);
  const std::vector<Node*> to_drops = collect_drop_nodes(to_first_node);
  const std::vector<Node*> from_block_nodes =
      slice_block(from_drops, from_block.first_unit, from_block.unit_count);
  const std::vector<Node*> to_block_nodes =
      slice_block(to_drops, to_block.first_unit, to_block.unit_count);
  if (!can_move_pick_activities(from_pick, from_block_nodes) ||
      !can_move_pick_activities(to_pick, to_block_nodes)) {
    return false;
  }
  // 各自插到对方块的原位置：锚点取原块首单元的前驱
  Node* anchor_from = from_drops[static_cast<std::size_t>(from_block.first_unit)]->prev;
  Node* anchor_to = to_drops[static_cast<std::size_t>(to_block.first_unit)]->prev;
  Node::UPtr from_chain = take_block(from_drops, from_block.first_unit, from_block.unit_count);
  Node::UPtr to_chain = take_block(to_drops, to_block.first_unit, to_block.unit_count);
  // 双向搬 pick activity：from 的订单去 to，to 的订单来 from
  move_pick_activities(from_pick, to_pick, from_block_nodes);
  move_pick_activities(to_pick, from_pick, to_block_nodes);
  NodeOps::splice_in_after(anchor_from, std::move(to_chain));
  NodeOps::splice_in_after(anchor_to, std::move(from_chain));
  return true;
}

bool reverse(Node::UPtr& first_node, const LsBlock& block) {
  if (!is_valid_block(first_node, block)) {
    return false;
  }
  const std::vector<Node*> drops = collect_drop_nodes(first_node);
  Node* anchor = slot_anchor(first_node, block.first_unit);
  if (anchor == nullptr) {
    return false;
  }
  Node::UPtr chain = take_block(drops, block.first_unit, block.unit_count);
  Node::UPtr reversed = NodeOps::reverse_chain(std::move(chain));
  NodeOps::splice_in_after(anchor, std::move(reversed));
  return true;
}

void canonicalize(Node::UPtr& first_node) {
  Node* pick_node = find_pick_node(first_node);
  if (pick_node == nullptr) {
    return;
  }
  Node* prev = pick_node;
  Node* cur = pick_node->next.get();
  while (cur != nullptr && cur->is_drop()) {
    Node* next = cur->next.get();
    if (cur->first == nullptr) {
      // 空 DROP 节点直接摘除
      NodeOps::splice_out(prev->next, cur);
    } else if (prev->is_drop() && prev->loc == cur->loc) {
      // 相邻同站点：把后者的 activity 逐个挂到前者，再摘除空节点
      while (cur->first != nullptr) {
        Activity::UPtr activity = detach_activity(cur, cur->first.get());
        if (activity == nullptr) {
          break;
        }
        prev->add_back_activity(std::move(activity));
      }
      NodeOps::splice_out(prev->next, cur);
    } else {
      prev = cur;
    }
    cur = next;
  }
}

}  // namespace LsNodeEdit
