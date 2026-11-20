/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <variant>

/**
 * @brief 邻域搜索中的一个块
 * @details 位置一律是「第几个可移动单元」，单元语义由 LsNeighborhood 适配器定义
 *          （SPMD 下是 drop 节点）。unit_count >= 1。
 */
struct LsBlock {
  int load_ind{-1};
  int first_unit{0};
  int unit_count{0};
};

/**
 * @brief 搬一个块
 * @details 跨车时槽位按目标当前序列计；同车时按移除源块后的序列计。
 */
struct LsRelocateMove {
  LsBlock block;
  int to_load{-1};
  int to_slot{0};
};

/**
 * @brief 交换两个块（各自占据对方的原位置），可同车可跨车
 */
struct LsExchangeMove {
  LsBlock first;
  LsBlock second;
};

/**
 * @brief 原地反转一个块（2-opt），仅同车
 */
struct LsReverseMove {
  LsBlock block;
};

/**
 * @brief 邻域搜索的统一 move 描述
 * @details 只含索引，不含 Load 或 Node 指针；算子只负责生成，引擎负责落到副本上。
 */
using LsMove = std::variant<LsRelocateMove, LsExchangeMove, LsReverseMove>;
