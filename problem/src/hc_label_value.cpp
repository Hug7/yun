/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#include "hc_label_value.h"

#include <bit>
#include <memory>
#include <span>
#include <utility>

#include "bdm_location.h"
#include "pdm_segment.h"

namespace {

using Mask = std::vector<std::uint64_t>;

void set_bit(Mask& mask, const int value_ind) {
  mask[static_cast<size_t>(value_ind) >> 6] |= (1ULL << (value_ind & 63));
}

/**
 * @brief 互斥模式：每个取值与同组内其它取值互斥；不同组不传递。
 * @details e.g. groups = [[0,1,2] [2,3]]
 * 拼装 group_masks
 * group_masks = [[1,1,1,0],[0,0,1,1]]
 * 拼装 masks
 * g = 0, value_ind = 0, mask[0]=[1,1,1,0]
 * g = 0, value_ind = 1, mask[1]=[1,1,1,0]
 * g = 0, value_ind = 2, mask[2]=[1,1,1,0]
 * g = 1, value_ind = 2, mask[2]=[1,1,1,1]
 * g = 1, value_ind = 3, mask[3]=[0,0,1,1]
 */
std::vector<Mask> build_exclusive_masks(const std::vector<std::vector<int>>& groups,
                                        const int value_count, const int words_len) {
  // masks[v] 是取值 v 的“不可拼位图”，即所有与 v 同组的值。组之间允许重叠，
  // 因此同一取值可以把多个组的位图并进来，但不会跨组传递。
  std::vector<Mask> masks(static_cast<size_t>(value_count),
                          Mask(static_cast<size_t>(words_len), 0));
  // 先为每个配置组单独建一张位图，避免后面重复遍历组内取值。
  std::vector<Mask> group_masks(groups.size(), Mask(static_cast<size_t>(words_len), 0));
  for (size_t g = 0; g < groups.size(); ++g) {
    for (const int value_ind : groups[g]) {
      set_bit(group_masks[g], value_ind);
    }
  }

  // 一个取值可能出现在多个组（跨组重复）：把它所在所有组的位图 OR 起来，
  // 得到该取值的完整不可拼集合；这里不做组的传递合并。
  for (size_t g = 0; g < groups.size(); ++g) {
    for (const int value_ind : groups[g]) {
      Mask& mask = masks[static_cast<size_t>(value_ind)];
      for (size_t w = 0; w < mask.size(); ++w) {
        mask[w] |= group_masks[g][w];
      }
    }
  }

  // 上面的 OR 把取值自身也包含进来了；清掉自身位，保证单值组的掩码为空，
  // 也避免判定时把同一个取值误判为与自身互斥。
  for (int value_ind = 0; value_ind < value_count; ++value_ind) {
    masks[static_cast<size_t>(value_ind)][static_cast<size_t>(value_ind) >> 6] &=
        ~(1ULL << (value_ind & 63));
  }
  return masks;
}

/**
 * @brief 可拼模式：同一生效组内可拼，不同生效组之间不可拼。
 */
std::vector<Mask> build_compatible_masks(const std::vector<std::vector<int>>& groups,
                                         const int value_count, const int words_len) {
  // masks[v] 是取值 v 的“不可拼位图”：与 v 不在同一个生效组的所有取值。
  // groups 已由解析层补齐默认组（未列出的取值），且保证各生效组两两不相交，
  // 因此每个取值恰好属于一个组。
  std::vector<Mask> masks(static_cast<size_t>(value_count),
                          Mask(static_cast<size_t>(words_len), 0));
  // group_masks[g] 是第 g 个生效组的位图；all 是所有生效组的并集，即全部取值。
  std::vector<Mask> group_masks(groups.size(), Mask(static_cast<size_t>(words_len), 0));
  Mask all(static_cast<size_t>(words_len), 0);
  for (size_t g = 0; g < groups.size(); ++g) {
    for (const int value_ind : groups[g]) {
      set_bit(group_masks[g], value_ind);
      set_bit(all, value_ind);
    }
  }

  // 对组 g 内的每个取值，不可拼集合 = 全部取值去掉组 g 自身。
  // 同组取值因此两两可拼，其它任何组的取值都不可拼；
  // 与互斥模式不同，这里不需要再清自身位，因为自身就在被去掉的组掩码里。
  for (size_t g = 0; g < groups.size(); ++g) {
    for (const int value_ind : groups[g]) {
      Mask& mask = masks[static_cast<size_t>(value_ind)];
      for (size_t w = 0; w < mask.size(); ++w) {
        mask[w] = all[w] & ~group_masks[g][w];
      }
    }
  }
  return masks;
}

/**
 * @brief 检查车次标签值集合是否与任一取值的不可拼位图相交。
 */
// 判断车次位图里是否存在“不可拼”的取值对：对每个已出现取值，用它的不可拼位图
// 与车次位图求交，非空即说明它与车次里另一个已出现取值冲突。
bool violates_masks(const LabelsetValueBitset* bitset, const LabelValueScope& scope,
                    const std::vector<Mask>& masks, const int value_count, const int words_len) {
  // 空位图或该标签的不同取值少于 2 个时，不可能构成“一对”，直接可行。
  if (bitset == nullptr || bitset->sum(scope.label_ind) < 2) {
    return false;
  }
  // load_words 是该标签在车次位图中的连续 word 区间，与预计算掩码使用同一布局。
  const auto load_words = bitset->label_words(scope.label_ind);
  if (load_words.size() != static_cast<size_t>(words_len)) {
    // 布局不一致（例如约束与车次来自不同 labelset）时不做判定，按可行处理。
    return false;
  }

  // 逐 word 扫描车次位图，只处理置位（即该车次实际包含的标签值）。
  for (size_t w = 0; w < load_words.size(); ++w) {
    std::uint64_t bits = load_words[w];
    while (bits != 0) {
      // 取当前最低置位，换算成该标签的取值下标。
      const int bit = std::countr_zero(bits);
      const int value_ind = static_cast<int>((w << 6) + static_cast<size_t>(bit));
      // 最后一个 word 的尾部可能有布局填充位，需排除。
      if (value_ind < value_count) {
        // masks[value_ind] 记录与它不可拼的所有取值；与车次位图逐 word 求交，
        // 有任意一个 word 非零就说明车次内存在冲突对，可立即返回。
        // 不可拼关系是对称的，因此只需从已出现的取值出发检查一遍。
        const Mask& mask = masks[static_cast<size_t>(value_ind)];
        for (size_t u = 0; u < load_words.size(); ++u) {
          if ((mask[u] & load_words[u]) != 0) {
            return true;
          }
        }
      }
      // 清掉最低置位，继续处理下一个已出现取值。
      bits &= bits - 1;
    }
  }
  return false;
}

/**
 * @brief 构造插入 segment 之后的标签值位图
 * @details 插入只会增加标签值, 因此基车次位图与 segment 涉及站点/订单的位图求并
 * 就是插入后的完整位图, 与具体插入位置无关。
 */
LabelsetValueBitset::UPtr make_post_bitset(const LabelValueScope& scope, Load* load,
                                           const Segment* segment, const bool pick) {
  LabelsetValueBitset::UPtr post_bitset = scope.labelset->empty_labelset_value_bitset();
  const LabelsetValueBitset* base_bitset = nullptr;
  if (scope.entity == LabelValueEntity::Order) {
    base_bitset = load->get_order_labelset_value_bitset();
  } else {
    base_bitset = pick ? load->get_pick_loc_labelset_value_bitset()
                       : load->get_drop_loc_labelset_value_bitset();
  }
  if (base_bitset != nullptr) {
    post_bitset->merge(*base_bitset);
  }

  for (const Order* order : segment->orders) {
    const LabelsetValueBitset* order_bitset = nullptr;
    if (scope.entity == LabelValueEntity::Order) {
      order_bitset = order->labelset_value_bitset.get();
    } else {
      const Location* loc = pick ? order->pick_loc : order->drop_loc;
      if (loc != nullptr) {
        order_bitset = loc->labelset_value_bitset.get();
      }
    }
    if (order_bitset != nullptr) {
      post_bitset->merge(*order_bitset);
    }
  }
  return post_bitset;
}

}  // namespace

// ====== implement of HcLabelValueExclusive ======
HcLabelValueExclusive::HcLabelValueExclusive(const LabelValueScope label_value_scope,
                                             const std::vector<std::vector<int>>& exclusive_groups)
    : HardConstraint(HardConstrCode::LabelValueExclusive, 0, false, false),
      scope(label_value_scope),
      groups(exclusive_groups),
      label_value_count_(label_value_scope.labelset->labels[label_value_scope.label_ind]->len),
      label_words_len_((label_value_count_ + 63) / 64),
      incompatible_masks_(build_exclusive_masks(groups, label_value_count_, label_words_len_)) {}

bool HcLabelValueExclusive::violates(const LabelsetValueBitset* bitset) const {
  return violates_masks(bitset, this->scope, this->incompatible_masks_, this->label_value_count_,
                        this->label_words_len_);
}

HardConstrScore::UPtr HcLabelValueExclusive::eval(Load* load) {
  const auto violation = [this](const InfeasibleReason& infeasible_reason) {
    return std::make_unique<HardConstrScore>(this->code, false, 1, 1, this->seq_type,
                                             this->vehicle_type, infeasible_reason);
  };
  if (this->scope.entity == LabelValueEntity::Order) {
    if (this->violates(load->get_order_labelset_value_bitset())) {
      return violation(InfeasibleReasonCollection::ORDER_LABEL_VALUE_EXCLUSIVE);
    }
  } else if (this->scope.side == LabelValueSide::Pick) {
    if (this->violates(load->get_pick_loc_labelset_value_bitset())) {
      return violation(InfeasibleReasonCollection::PICK_LOCATION_LABEL_VALUE_EXCLUSIVE);
    }
  } else if (this->scope.side == LabelValueSide::Drop) {
    if (this->violates(load->get_drop_loc_labelset_value_bitset())) {
      return violation(InfeasibleReasonCollection::DROP_LOCATION_LABEL_VALUE_EXCLUSIVE);
    }
  } else {
    if (this->violates(load->get_pick_loc_labelset_value_bitset())) {
      return violation(InfeasibleReasonCollection::PICK_LOCATION_LABEL_VALUE_EXCLUSIVE);
    }
    if (this->violates(load->get_drop_loc_labelset_value_bitset())) {
      return violation(InfeasibleReasonCollection::DROP_LOCATION_LABEL_VALUE_EXCLUSIVE);
    }
  }
  return std::make_unique<HardConstrScore>(this->code);
}

HcPrecheckVerdict HcLabelValueExclusive::precheck(const HcPrecheckInput& input) const {
  if (this->scope.entity == LabelValueEntity::Order) {
    return this->violates(make_post_bitset(this->scope, input.load, input.segment, true).get())
               ? HcPrecheckVerdict::kInfeasible
               : HcPrecheckVerdict::kPass;
  }
  // Location: 按侧别分别检查提货侧与卸货侧, Both 时两侧都要检查
  if (this->scope.side != LabelValueSide::Drop &&
      this->violates(make_post_bitset(this->scope, input.load, input.segment, true).get())) {
    return HcPrecheckVerdict::kInfeasible;
  }
  if (this->scope.side != LabelValueSide::Pick &&
      this->violates(make_post_bitset(this->scope, input.load, input.segment, false).get())) {
    return HcPrecheckVerdict::kInfeasible;
  }
  return HcPrecheckVerdict::kPass;
}

// ====== implement of HcLabelValueCompatible ======
HcLabelValueCompatible::HcLabelValueCompatible(
    const LabelValueScope label_value_scope, const std::vector<std::vector<int>>& compatible_groups)
    : HardConstraint(HardConstrCode::LabelValueCompatible, 0, false, false),
      scope(label_value_scope),
      groups(append_unlisted_group(label_value_scope.labelset, label_value_scope.label_ind,
                                   compatible_groups)),
      label_value_count_(label_value_scope.labelset->labels[label_value_scope.label_ind]->len),
      label_words_len_((label_value_count_ + 63) / 64),
      incompatible_masks_(build_compatible_masks(groups, label_value_count_, label_words_len_)) {}

std::vector<std::vector<int>> HcLabelValueCompatible::append_unlisted_group(
    const Labelset* labelset, const int label_ind, const std::vector<std::vector<int>>& groups) {
  std::vector<std::vector<int>> result = groups;
  const Label* label = labelset->labels[label_ind];
  std::vector<bool> listed(label->len, false);
  for (const auto& group : groups) {
    for (const int value_ind : group) {
      listed[value_ind] = true;
    }
  }

  std::vector<int> unlisted;
  for (int value_ind = 0; value_ind < label->len; ++value_ind) {
    if (!listed[value_ind]) {
      unlisted.push_back(value_ind);
    }
  }
  if (!unlisted.empty()) {
    result.push_back(std::move(unlisted));
  }
  return result;
}

bool HcLabelValueCompatible::violates(const LabelsetValueBitset* bitset) const {
  return violates_masks(bitset, this->scope, this->incompatible_masks_, this->label_value_count_,
                        this->label_words_len_);
}

HardConstrScore::UPtr HcLabelValueCompatible::eval(Load* load) {
  const auto violation = [this](const InfeasibleReason& infeasible_reason) {
    return std::make_unique<HardConstrScore>(this->code, false, 1, 1, this->seq_type,
                                             this->vehicle_type, infeasible_reason);
  };
  if (this->scope.entity == LabelValueEntity::Order) {
    if (this->violates(load->get_order_labelset_value_bitset())) {
      return violation(InfeasibleReasonCollection::ORDER_LABEL_VALUE_COMPATIBLE);
    }
  } else if (this->scope.side == LabelValueSide::Pick) {
    if (this->violates(load->get_pick_loc_labelset_value_bitset())) {
      return violation(InfeasibleReasonCollection::PICK_LOCATION_LABEL_VALUE_COMPATIBLE);
    }
  } else if (this->scope.side == LabelValueSide::Drop) {
    if (this->violates(load->get_drop_loc_labelset_value_bitset())) {
      return violation(InfeasibleReasonCollection::DROP_LOCATION_LABEL_VALUE_COMPATIBLE);
    }
  } else {
    if (this->violates(load->get_pick_loc_labelset_value_bitset())) {
      return violation(InfeasibleReasonCollection::PICK_LOCATION_LABEL_VALUE_COMPATIBLE);
    }
    if (this->violates(load->get_drop_loc_labelset_value_bitset())) {
      return violation(InfeasibleReasonCollection::DROP_LOCATION_LABEL_VALUE_COMPATIBLE);
    }
  }
  return std::make_unique<HardConstrScore>(this->code);
}

HcPrecheckVerdict HcLabelValueCompatible::precheck(const HcPrecheckInput& input) const {
  if (this->scope.entity == LabelValueEntity::Order) {
    return this->violates(make_post_bitset(this->scope, input.load, input.segment, true).get())
               ? HcPrecheckVerdict::kInfeasible
               : HcPrecheckVerdict::kPass;
  }
  if (this->scope.side != LabelValueSide::Drop &&
      this->violates(make_post_bitset(this->scope, input.load, input.segment, true).get())) {
    return HcPrecheckVerdict::kInfeasible;
  }
  if (this->scope.side != LabelValueSide::Pick &&
      this->violates(make_post_bitset(this->scope, input.load, input.segment, false).get())) {
    return HcPrecheckVerdict::kInfeasible;
  }
  return HcPrecheckVerdict::kPass;
}

// ====== implement of HcLabelValueMaxCount ======
HcLabelValueMaxCount::HcLabelValueMaxCount(const LabelValueScope label_value_scope,
                                           const int max_count)
    : HardConstraint(HardConstrCode::LabelValueMaxCount, 0, false, false),
      scope(label_value_scope),
      max_count(max_count),
      label_words_len_((label_value_scope.labelset->labels[label_value_scope.label_ind]->len + 63) /
                       64) {}

bool HcLabelValueMaxCount::violates(const LabelsetValueBitset* bitset) const {
  if (bitset == nullptr) {
    return false;
  }
  return bitset->sum(this->scope.label_ind) > this->max_count;
}

bool HcLabelValueMaxCount::violates_union(const LabelsetValueBitset* pick_bitset,
                                          const LabelsetValueBitset* drop_bitset) const {
  const std::span<const std::uint64_t> pick_words =
      pick_bitset == nullptr ? std::span<const std::uint64_t>{}
                             : pick_bitset->label_words(this->scope.label_ind);
  const std::span<const std::uint64_t> drop_words =
      drop_bitset == nullptr ? std::span<const std::uint64_t>{}
                             : drop_bitset->label_words(this->scope.label_ind);
  // 位图与约束来自不同 labelset（word 布局不一致）时不做判定，按可行处理；
  // 与 HcLabelValueExclusive/HcLabelValueCompatible 的布局校验保持一致。
  const auto expected_words = static_cast<size_t>(this->label_words_len_);
  if ((pick_bitset != nullptr && pick_words.size() != expected_words) ||
      (drop_bitset != nullptr && drop_words.size() != expected_words)) {
    return false;
  }

  // 合并去重：两侧逐 word 求并后统计置位数，同一个取值在两侧出现只计一次。
  int count = 0;
  for (size_t w = 0; w < expected_words; ++w) {
    const std::uint64_t pick = w < pick_words.size() ? pick_words[w] : 0;
    const std::uint64_t drop = w < drop_words.size() ? drop_words[w] : 0;
    count += std::popcount(pick | drop);
    if (count > this->max_count) {
      return true;
    }
  }
  return false;
}

HardConstrScore::UPtr HcLabelValueMaxCount::eval(Load* load) {
  const auto violation = [this](const InfeasibleReason& infeasible_reason) {
    return std::make_unique<HardConstrScore>(this->code, false, 1, 1, this->seq_type,
                                             this->vehicle_type, infeasible_reason);
  };
  if (this->scope.entity == LabelValueEntity::Order) {
    if (this->violates(load->get_order_labelset_value_bitset())) {
      return violation(InfeasibleReasonCollection::ORDER_LABEL_VALUE_MAX_COUNT);
    }
  } else if (this->scope.side == LabelValueSide::Pick) {
    if (this->violates(load->get_pick_loc_labelset_value_bitset())) {
      return violation(InfeasibleReasonCollection::PICK_LOCATION_LABEL_VALUE_MAX_COUNT);
    }
  } else if (this->scope.side == LabelValueSide::Drop) {
    if (this->violates(load->get_drop_loc_labelset_value_bitset())) {
      return violation(InfeasibleReasonCollection::DROP_LOCATION_LABEL_VALUE_MAX_COUNT);
    }
  } else {
    if (this->violates_union(load->get_pick_loc_labelset_value_bitset(),
                             load->get_drop_loc_labelset_value_bitset())) {
      return violation(InfeasibleReasonCollection::LOCATION_LABEL_VALUE_MAX_COUNT);
    }
  }
  return std::make_unique<HardConstrScore>(this->code);
}

HcPrecheckVerdict HcLabelValueMaxCount::precheck(const HcPrecheckInput& input) const {
  if (this->scope.entity == LabelValueEntity::Order) {
    return this->violates(make_post_bitset(this->scope, input.load, input.segment, true).get())
               ? HcPrecheckVerdict::kInfeasible
               : HcPrecheckVerdict::kPass;
  }
  if (this->scope.side == LabelValueSide::Both) {
    // 提货侧与卸货侧合并去重后计数, 与 eval 的判定口径一致
    const auto post_pick = make_post_bitset(this->scope, input.load, input.segment, true);
    const auto post_drop = make_post_bitset(this->scope, input.load, input.segment, false);
    return this->violates_union(post_pick.get(), post_drop.get()) ? HcPrecheckVerdict::kInfeasible
                                                                  : HcPrecheckVerdict::kPass;
  }
  const bool pick = this->scope.side == LabelValueSide::Pick;
  return this->violates(make_post_bitset(this->scope, input.load, input.segment, pick).get())
             ? HcPrecheckVerdict::kInfeasible
             : HcPrecheckVerdict::kPass;
}
