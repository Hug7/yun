/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <cstdint>
#include <vector>

#include "bdm_label.h"
#include "hc_constraint.h"

/**
 * @brief 标签值约束作用的实体域
 */
enum class LabelValueEntity {
  Location,
  Order,
};

/**
 * @brief Location 站点侧别：提货侧、卸货侧或两侧
 */
enum class LabelValueSide {
  Both,
  Pick,
  Drop,
};

/**
 * @brief 标签值约束的作用范围
 * @details labelset 必须是 scenario 中 Location / Order 标签集合之一，
 * label_ind 是该标签在 labelset 中的索引。side 仅对 Location 生效。
 */
struct LabelValueScope {
  const Labelset* labelset;
  int label_ind;
  LabelValueEntity entity;
  LabelValueSide side = LabelValueSide::Both;
};

/**
 * @brief 硬约束-标签值互斥
 * @details 每个组内的标签值两两不可同车；不同组之间不做传递合并，
 * 只有同时出现在某一组中的值对才互斥。未出现在任何组中的标签值不受约束。
 */
class HcLabelValueExclusive : public HardConstraint {
 public:
  const LabelValueScope scope;
  /**
   * @brief 互斥组，每个组内的标签值两两不可同车
   */
  const std::vector<std::vector<int>> groups;

  HcLabelValueExclusive(LabelValueScope scope,
                        const std::vector<std::vector<int>>& exclusive_groups);

  HardConstrScore::UPtr eval(Load* load) override;

 private:
  [[nodiscard]] bool violates(const LabelsetValueBitset* bitset) const;

  /**
   * @brief 标签取值数量
   */
  const int label_value_count_;
  /**
   * @brief 本标签在位图 arena 中占用的 word 数
   */
  const int label_words_len_;
  /**
   * @brief 每个标签取值对应的不可拼位图，位图内只使用本标签的取值位
   * @details incompatible_masks_[v] 中为 1 的位表示与取值 v 不可同车的标签取值。
   */
  const std::vector<std::vector<std::uint64_t>> incompatible_masks_;
};

/**
 * @brief 硬约束-标签值可拼
 * @details 每个组内的标签值可同车，不同组之间不可同车。未出现在任何组中的
 * 标签值在构造时汇总成一个默认组，该组内仍可同车。
 */
class HcLabelValueCompatible : public HardConstraint {
 public:
  const LabelValueScope scope;
  /**
   * @brief 生效组，显式组之后是未列出标签值的默认组（若存在）
   */
  const std::vector<std::vector<int>> groups;

  HcLabelValueCompatible(LabelValueScope scope,
                         const std::vector<std::vector<int>>& compatible_groups);

  HardConstrScore::UPtr eval(Load* load) override;

 private:
  [[nodiscard]] bool violates(const LabelsetValueBitset* bitset) const;

  static std::vector<std::vector<int>> append_unlisted_group(
      const Labelset* labelset, int label_ind, const std::vector<std::vector<int>>& groups);

  /**
   * @brief 标签取值数量
   */
  const int label_value_count_;
  /**
   * @brief 本标签在位图 arena 中占用的 word 数
   */
  const int label_words_len_;
  /**
   * @brief 每个标签取值对应的不可拼位图，位图内只使用本标签的取值位
   * @details incompatible_masks_[v] 中为 1 的位表示与取值 v 不可同车的标签取值。
   */
  const std::vector<std::vector<std::uint64_t>> incompatible_masks_;
};

/**
 * @brief 硬约束-标签值种类数上限
 * @details 统计车次内指定标签出现过的不同取值个数，超过 max_count 即不可行。
 * entity 为 Location 且 side 为 Both 时，按提货侧与卸货侧合并去重后计数。
 */
class HcLabelValueMaxCount : public HardConstraint {
 public:
  const LabelValueScope scope;
  /**
   * @brief 允许的标签值种类数上限
   */
  const int max_count;

  HcLabelValueMaxCount(LabelValueScope scope, int max_count);

  HardConstrScore::UPtr eval(Load* load) override;

 private:
  [[nodiscard]] bool violates(const LabelsetValueBitset* bitset) const;

  /**
   * @brief 提货侧与卸货侧标签值合并去重后的种类数是否超过上限
   */
  [[nodiscard]] bool violates_union(const LabelsetValueBitset* pick_bitset,
                                    const LabelsetValueBitset* drop_bitset) const;

  /**
   * @brief 本标签在位图 arena 中占用的 word 数
   */
  const int label_words_len_;
};
