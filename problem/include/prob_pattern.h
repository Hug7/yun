/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <cstdint>
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

  InsertSlot(const ActivityType _node_type, const int _ind) : node_type(_node_type), ind(_ind) {}
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

  /**
   * @brief 估算 segment 插入各槽位后连接处的距离变化
   * @details 只依据插入位置与当前车型的距离矩阵估算, 不修改 load, 不检查任何约束, 也不创建
   * node/activity。不模拟同站点合并: 直接把 segment 当成夹在 anchor 与 successor 之间的整块,
   * 只取 segment 首/尾 drop 站点的两条连接边, 作为槽位排序用的启发式值。
   * @param load 要操作的车次
   * @param segment 要插入的segment
   * @param slots 由 enumerate_insert_slots 返回的槽位集合
   * @return 与 slots 一一对应的距离变化
   */
  virtual std::vector<std::int64_t> estimate_insert_delta_dists(
      Load* load, const Segment* segment, const std::vector<InsertSlot>& slots) const = 0;

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

  std::vector<std::int64_t> estimate_insert_delta_dists(
      Load* load, const Segment* segment, const std::vector<InsertSlot>& slots) const override;

  /**
   * @brief 估算把 segment 整块插入 anchor 之后, 两端连接处的距离变化
   * @details 不模拟同站点合并, 直接用 segment 首/尾 drop 站点与 anchor、successor 算两条连接边,
   * 作为槽位排序用的启发式值。全程只读, 不创建 node/activity。
   * @param dist_matrix 当前车型的距离矩阵
   * @param anchor 新片段插在 anchor 之后
   * @param first_drop_loc segment 内第一个 drop 站点
   * @param last_drop_loc segment 内最后一个 drop 站点
   */
  static std::int64_t estimate_drop_delta_dist(const DistMatrix& dist_matrix, const Node* anchor,
                                               const Location* first_drop_loc,
                                               const Location* last_drop_loc);
};
