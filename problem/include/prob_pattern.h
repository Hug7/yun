/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <memory>
#include <vector>

#include "pdm_load.h"
#include "pdm_node.h"
#include "pdm_segment.h"

/**
 * @brief segment 在 load 中的插入位置
 * @details node_type 指定插入区(PICK 区/DROP 区); index 表示"插入后, 新片段前/后剩余的原有 node
 * 数": PICK 区从前往后数(新片段前面剩余 index 个 pick node), DROP 区从后往前数(新片段后面剩余 index
 * 个 drop node)。取值区间为 [0, 该区 node 数], 首尾 depot 哨兵不参与计数。
 */
struct InsertSlot {
  ActivityType node_type;
  int ind;

  InsertSlot() : node_type(ActivityType::DROP), ind(-1) {}

  InsertSlot(const ActivityType _node_type, const int _ind)
      : node_type(_node_type), ind(_ind) {}
};

class PickDropPattern {
 public:
  virtual ~PickDropPattern() = default;

  virtual Node* find_first_pick_node(std::unique_ptr<Node>& node) const;

  virtual Node* find_last_pick_node(std::unique_ptr<Node>& node) const;

  virtual Node* find_first_drop_node(std::unique_ptr<Node>& node) const;

  virtual Node* find_last_drop_node(std::unique_ptr<Node>& node) const;

  virtual Node* find_load_first_pick_node(Load* load) const;

  virtual Node* find_load_last_pick_node(Load* load) const;

  virtual Node* find_load_first_drop_node(Load* load) const;

  virtual Node* find_load_last_drop_node(Load* load) const;

  virtual Load* create_load(LoadContext* context) const = 0;

  virtual Load* deep_copy_load(const Load* other) const = 0;

  virtual bool insert_last_delivery(Load* load, const Order* order) const = 0;

  /**
   * @brief 枚举 segment 可插入 load 的位置
   * @return 空集合表示该 segment 无法插入此 load
   */
  virtual std::vector<InsertSlot> enumerate_insert_slots(Load* load,
                                                         const Segment* segment) const = 0;

  /**
   * @brief 将 segment 插入 load 的指定位置
   * @return 本次新增的 activities(提+卸), 供调用方 remove_activities 精确回滚; 空表示插入失败
   */
  virtual std::vector<Activity*> insert_segment_at(Load* load, const Segment* segment,
                                                   const InsertSlot& slot) const = 0;

  PickDropPattern() = default;
};

class PatternSPMD : public PickDropPattern {
 public:
  PatternSPMD() : PickDropPattern() {}

  Node* find_load_first_pick_node(Load* load) const override;

  Node* find_load_last_pick_node(Load* load) const override;

  Node* find_load_first_drop_node(Load* load) const override;

  Node* find_load_last_drop_node(Load* load) const override;

  LoadSPMD* create_load(LoadContext* context) const override;

  LoadSPMD* deep_copy_load(const Load* other) const override;

  bool insert_last_delivery(Load* load, const Order* order) const override;

  std::vector<InsertSlot> enumerate_insert_slots(Load* load, const Segment* segment) const override;

  std::vector<Activity*> insert_segment_at(Load* load, const Segment* segment,
                                           const InsertSlot& slot) const override;
};
