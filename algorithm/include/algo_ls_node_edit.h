/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <vector>

#include "algo_ls_move.h"
#include "pdm_activity.h"
#include "pdm_node.h"

/**
 * @brief SPMD 结构的 node/activity 编辑原语
 * @details 只操作 Node/Activity 链本身，不访问 Load/Solution/Problem，也不调用
 *          `insert_segment_at` / `eval_load`，因此可以脱离完整 Scenario 直接单测。
 *
 *          链形契约（SPMD）：first_node(START 哨兵) -> 1 个 PICK 节点 -> k 个 DROP 节点 ->
 *          last_node(END 哨兵)。"单元" = PICK 之后的第 i 个 DROP 节点，head-relative 下标
 *          0..k-1；槽位 s 表示"插入后其前面有 s 个剩余单元"，s ∈ [0, 剩余单元数]。
 *
 *          所有函数都不分配 Node/Activity（只做链表与 activity 摘挂），移动后 Activity 对象
 *          指针保持不变；`pick`/`drop` 的 `related` 互指随对象一起保留。
 */
namespace LsNodeEdit {

/**
 * @brief 取 PICK 节点（first_node->next）
 * @return 链首不是 PICK（空 Load / 非 SPMD 链）时返回 nullptr
 */
Node* find_pick_node(const Node::UPtr& first_node);

/**
 * @brief DROP 单元数
 * @details 从 PICK 之后直接沿链计数，不依赖 LoadRouteProfile 的缓存，因此拿到的一定是当前
 *          结构的最新值；空 Load 返回 0。
 */
int drop_node_count(const Node::UPtr& first_node);

/**
 * @brief 收集 DROP 节点指针，head-relative 升序
 */
std::vector<Node*> collect_drop_nodes(const Node::UPtr& first_node);

/**
 * @brief 校验块范围：unit_count >= 1、first_unit >= 0、first_unit + unit_count <= 单元数
 */
bool is_valid_block(const Node::UPtr& first_node, const LsBlock& block);

/**
 * @brief 槽位对应的锚点：新子链插在该节点之后
 * @details slot == 0 时锚点为 PICK 节点（插在全部 DROP 之前），否则为第 slot 个 DROP 节点
 *          （1-based），即剩余序列里"前 slot 个单元"的最后一个。越界返回 nullptr。
 */
Node* slot_anchor(const Node::UPtr& first_node, int slot);

/**
 * @brief 从 node 的 activity 链上摘下 activity，并转移所有权（不销毁 activity）
 * @details `Node::remove_activity` 会在 unique_ptr 移动赋值时销毁 activity，无法用于跨 node
 *          搬运同一 Activity 对象，因此这里用同样的链表操作但把所有权返回给调用方。
 * @return 失败（不属于该 node 等）返回 nullptr
 */
Activity::UPtr detach_activity(Node* node, Activity* activity);

/**
 * @brief 同车搬块（relocate / or-opt）：摘出 DROP 子链，按移除后序列的槽位插回
 * @details 不触碰 PICK 节点与 pick activity。to_slot 按移除源块后的序列计。
 */
bool relocate_intra(Node::UPtr& first_node, const LsBlock& block, int to_slot);

/**
 * @brief 跨车搬块：摘出源 DROP 子链，并把块内订单的 pick activity 从源 PICK 摘到目标 PICK
 * @details 目标插入位按目标当前序列计。两车 PICK 站点必须相同（SPMD 约束）；v1 不允许把
 *          源车次搬空（block 覆盖全部单元时返回 false）。
 */
bool relocate_cross(Node::UPtr& from_first_node, Node::UPtr& to_first_node, const LsBlock& block,
                    int to_slot);

/**
 * @brief 同车交换两个不相交块：右块占左块原位，左块插到右块原位（中间单元保持相对顺序）
 * @details 不触碰 PICK 节点与 pick activity。
 */
bool exchange_intra(Node::UPtr& first_node, const LsBlock& first, const LsBlock& second);

/**
 * @brief 跨车交换两个块：各自插到对方块的原位置，并双向搬 pick activity
 * @details 两车 PICK 站点必须相同（SPMD 约束）。
 */
bool exchange_cross(Node::UPtr& from_first_node, Node::UPtr& to_first_node,
                    const LsBlock& from_block, const LsBlock& to_block);

/**
 * @brief 原地反转一个块（2-opt）：摘链 -> NodeOps::reverse_chain -> 插回原锚点（仅同车）
 */
bool reverse(Node::UPtr& first_node, const LsBlock& block);

/**
 * @brief 恢复 SPMD 规范形：合并相邻且 loc 指针相同的 DROP 节点，摘除空 DROP 节点
 * @details 合并时把后者的 activity 逐个挂到前者（对象不重建），并修正 owner_node/related 归属。
 */
void canonicalize(Node::UPtr& first_node);

}  // namespace LsNodeEdit
