/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#include <algorithm>
#include <vector>

#include "bdm_charge.h"

namespace {

/**
 * @brief 非负整数除法，四舍五入
 */
long div_round(const __int128 numerator, const long denominator) {
  return static_cast<long>((numerator + denominator / 2) / denominator);
}

/**
 * @brief 线路标签值序列
 */
struct ChargePathSeq {
  /**
   * @brief 每个停靠点的值区间偏移，长度为停靠点数 + 1
   */
  std::vector<int> offsets;
  /**
   * @brief 摊平后的标签值索引
   */
  std::vector<int> values;
};

/**
 * @brief 判断某个停靠点是否包含指定标签值
 */
bool slice_contains(const ChargePathSeq& seq, const int stop, const int value) {
  const int begin = seq.offsets[stop];
  const int end = seq.offsets[stop + 1];
  for (int u = begin; u < end; u++) {
    if (seq.values[u] == value) {
      return true;
    }
  }
  return false;
}

/**
 * @brief 订单 OD 对编码
 * @details 高 32 位为提货标签值，低 32 位为卸货标签值
 */
uint64_t make_od_key(const int pick_value, const int drop_value) {
  return (static_cast<uint64_t>(static_cast<uint32_t>(pick_value)) << 32) |
         static_cast<uint32_t>(drop_value);
}

/**
 * @brief 单个车次允许枚举的 OD 对上限，避免多值标签组合爆炸
 */
constexpr size_t MAX_OD_PAIRS = 4096;

/**
 * @brief 线程本地 scratch
 * @details 并行求解时每个工作线程各用一份，ChargeManager 本身保持只读
 */
thread_local std::vector<ChargePathSeq> tls_path_seqs;
thread_local std::vector<std::vector<uint64_t>> tls_od_pairs;
thread_local std::vector<uint8_t> tls_od_slot_valid;
thread_local std::vector<std::vector<uint8_t>> tls_secondary_covered;
thread_local std::vector<std::vector<long>> tls_secondary_pair_sums;
thread_local std::vector<long> tls_secondary_other_sums;
thread_local std::vector<uint32_t> tls_secondary_epoch;  // 次级编码 i 最后一次被“归零”的代数
thread_local std::vector<long> tls_charge_code_max;
thread_local std::vector<uint32_t> tls_charge_code_epoch;  // 计费编码 i 最后一次被“归零”的代数
thread_local std::vector<int> tls_touched_secondaries;
thread_local std::vector<int> tls_touched_charge_codes;
thread_local uint32_t tls_epoch = 0;  // 当前是第几次 evaluate

}  // namespace

/**
 * @brief 车次计费求值器
 * @details
 * 一次 evaluate 对应一个实例，负责把“线路匹配 → 阶梯匹配 → 计费项计算 → 汇总”
 * 四步拆开。实例只持有本次求值的引用与缓存，不引入堆分配。
 */
class ChargeEvaluator {
 public:
  ChargeEvaluator(const ChargeManager& manager, const ChargeLoadView& view)
      : manager(manager), view(view) {}

  [[nodiscard]] ChargeResult run();

 private:
  const ChargeManager& manager;
  const ChargeLoadView& view;
  int vehicle_ind{-1};

  bool distance_loaded_{false};
  long distance_value_{0};
  bool pick_loaded_{false};
  int pick_value_{0};
  bool drop_loaded_{false};
  int drop_value_{0};
  bool dims_loaded_{false};
  const long* dims_value_{nullptr};

  [[nodiscard]] long get_distance();
  [[nodiscard]] int get_pick_count();
  [[nodiscard]] int get_drop_count();
  [[nodiscard]] const long* get_dim_vals();

  void build_od_pairs();
  void build_ordered_seqs();
  void prepare_aggregation();
  void touch_secondary(int secondary_ind);

  void eval_rules();
  void eval_rule(int rule_ind);
  [[nodiscard]] bool match_path(const ChargeRule& rule, int& od_pair_ind);
  [[nodiscard]] bool match_tier(const ChargeRule& rule, const ChargeTier*& tier);
  [[nodiscard]] long calc_items(const ChargeTier& tier);
  void accumulate(int secondary_ind, int od_pair_ind, long amount);

  [[nodiscard]] ChargeResult finalize();
};

ChargeResult ChargeEvaluator::run() {
  if (!this->manager.enabled) {
    return ChargeResult{};
  }
  this->vehicle_ind = this->view.vehicle_ind();
  if (this->vehicle_ind < 0 ||
      this->vehicle_ind + 1 >= static_cast<int>(this->manager.vehicle_rule_offsets.size())) {
    return ChargeResult{};
  }

  this->build_od_pairs();
  this->build_ordered_seqs();
  this->prepare_aggregation();
  this->eval_rules();
  return this->finalize();
}

long ChargeEvaluator::get_distance() {
  if (!this->distance_loaded_) {
    this->distance_value_ = this->view.distance_m();
    this->distance_loaded_ = true;
  }
  return this->distance_value_;
}

int ChargeEvaluator::get_pick_count() {
  if (!this->pick_loaded_) {
    this->pick_value_ = this->view.pick_node_count();
    this->pick_loaded_ = true;
  }
  return this->pick_value_;
}

int ChargeEvaluator::get_drop_count() {
  if (!this->drop_loaded_) {
    this->drop_value_ = this->view.drop_node_count();
    this->drop_loaded_ = true;
  }
  return this->drop_value_;
}

const long* ChargeEvaluator::get_dim_vals() {
  if (!this->dims_loaded_) {
    this->dims_value_ = this->view.dim_vals();
    this->dims_loaded_ = true;
  }
  return this->dims_value_;
}

void ChargeEvaluator::build_od_pairs() {
  if (this->manager.od_path_label_slots.empty()) {
    return;
  }

  tls_od_pairs.resize(this->manager.path_label_count);
  tls_od_slot_valid.assign(this->manager.path_label_count, 0);

  const int order_count = this->view.order_count();
  const Location* const* order_pick_locs = order_count > 0 ? this->view.order_pick_locs() : nullptr;
  const Location* const* order_drop_locs = order_count > 0 ? this->view.order_drop_locs() : nullptr;

  for (const int slot : this->manager.od_path_label_slots) {
    std::vector<uint64_t>& pairs = tls_od_pairs[slot];
    pairs.clear();
    bool valid = order_count > 0 && order_pick_locs != nullptr && order_drop_locs != nullptr;
    if (valid) {
      for (int order_ind = 0; order_ind < order_count; order_ind++) {
        const Location* pick_loc = order_pick_locs[order_ind];
        const Location* drop_loc = order_drop_locs[order_ind];
        if (pick_loc == nullptr || drop_loc == nullptr) {
          valid = false;
          break;
        }
        const int pick_base = pick_loc->ind * this->manager.path_label_count + slot;
        const int drop_base = drop_loc->ind * this->manager.path_label_count + slot;
        const int pick_begin = this->manager.loc_label_offsets[pick_base];
        const int pick_end = this->manager.loc_label_offsets[pick_base + 1];
        const int drop_begin = this->manager.loc_label_offsets[drop_base];
        const int drop_end = this->manager.loc_label_offsets[drop_base + 1];
        if (pick_begin == pick_end || drop_begin == drop_end) {
          valid = false;
          break;
        }
        for (int pick_ind = pick_begin; pick_ind < pick_end && valid; pick_ind++) {
          for (int drop_ind = drop_begin; drop_ind < drop_end; drop_ind++) {
            pairs.push_back(make_od_key(this->manager.loc_label_values[pick_ind],
                                        this->manager.loc_label_values[drop_ind]));
            if (pairs.size() > MAX_OD_PAIRS) {
              valid = false;
              break;
            }
          }
        }
        if (!valid) {
          break;
        }
      }
    }
    if (valid) {
      std::ranges::sort(pairs);
      pairs.erase(std::ranges::unique(pairs).begin(), pairs.end());
      tls_od_slot_valid[slot] = 1;
    } else {
      pairs.clear();
    }
  }
}

void ChargeEvaluator::build_ordered_seqs() {
  if (this->manager.ordered_path_label_slots.empty()) {
    return;
  }

  tls_path_seqs.resize(this->manager.path_label_count);
  for (const int slot : this->manager.ordered_path_label_slots) {
    tls_path_seqs[slot].offsets.clear();
    tls_path_seqs[slot].values.clear();
  }

  const int stop_count = this->view.stop_count();
  const Location* const* stops = stop_count > 0 ? this->view.stops() : nullptr;
  if (stop_count <= 0 || stops == nullptr) {
    return;
  }

  for (const int slot : this->manager.ordered_path_label_slots) {
    ChargePathSeq& seq = tls_path_seqs[slot];
    seq.offsets.push_back(0);
    seq.values.reserve(static_cast<size_t>(stop_count));
    for (int stop = 0; stop < stop_count; stop++) {
      const int location_ind = stops[stop]->ind;
      const int base = location_ind * this->manager.path_label_count + slot;
      const int begin = this->manager.loc_label_offsets[base];
      const int end = this->manager.loc_label_offsets[base + 1];
      seq.values.insert(seq.values.end(), this->manager.loc_label_values.begin() + begin,
                        this->manager.loc_label_values.begin() + end);
      const int new_begin = seq.offsets.back();
      const int new_end = static_cast<int>(seq.values.size());
      // 合并连续相同的标签值集合
      if (seq.offsets.size() >= 2) {
        const int prev_begin = seq.offsets[seq.offsets.size() - 2];
        const int prev_end = seq.offsets.back();
        if (new_end - new_begin == prev_end - prev_begin &&
            std::equal(seq.values.begin() + new_begin, seq.values.begin() + new_end,
                       seq.values.begin() + prev_begin)) {
          seq.values.resize(new_begin);
          continue;
        }
      }
      seq.offsets.push_back(new_end);
    }
  }
}

void ChargeEvaluator::prepare_aggregation() {
  const int secondary_count = static_cast<int>(this->manager.secondary_codes.size());
  const int charge_code_count = static_cast<int>(this->manager.charge_code_codes.size());

  if (static_cast<int>(tls_secondary_other_sums.size()) < secondary_count) {
    tls_secondary_other_sums.resize(secondary_count, 0);
    tls_secondary_epoch.resize(secondary_count, 0);
  }
  if (static_cast<int>(tls_charge_code_max.size()) < charge_code_count) {
    tls_charge_code_max.resize(charge_code_count, 0);
    tls_charge_code_epoch.resize(charge_code_count, 0);
  }
  if (++tls_epoch == 0) {
    std::ranges::fill(tls_secondary_epoch, 0);
    std::ranges::fill(tls_charge_code_epoch, 0);
    tls_epoch = 1;
  }

  tls_touched_secondaries.clear();
  tls_touched_charge_codes.clear();
  tls_secondary_covered.resize(secondary_count);
  tls_secondary_pair_sums.resize(secondary_count);
  for (const int secondary_ind : this->manager.od_secondary_inds) {
    const int slot = this->manager.secondary_od_slot[secondary_ind];
    const size_t pair_count = tls_od_pairs[slot].size();
    tls_secondary_covered[secondary_ind].assign(pair_count, 0);
    tls_secondary_pair_sums[secondary_ind].assign(pair_count, 0);
  }
}

void ChargeEvaluator::touch_secondary(const int secondary_ind) {
  if (tls_secondary_epoch[secondary_ind] != tls_epoch) {
    tls_secondary_epoch[secondary_ind] = tls_epoch;
    tls_secondary_other_sums[secondary_ind] = 0;
    tls_touched_secondaries.push_back(secondary_ind);
  }
}

void ChargeEvaluator::eval_rules() {
  for (const int rule_ind : this->manager.generic_rule_inds) {
    this->eval_rule(rule_ind);
  }
  const int vehicle_rule_begin = this->manager.vehicle_rule_offsets[this->vehicle_ind];
  const int vehicle_rule_end = this->manager.vehicle_rule_offsets[this->vehicle_ind + 1];
  for (int u = vehicle_rule_begin; u < vehicle_rule_end; u++) {
    this->eval_rule(this->manager.vehicle_rule_inds[u]);
  }
}

void ChargeEvaluator::eval_rule(const int rule_ind) {
  int od_pair_ind = -1;
  const ChargeRule& rule = this->manager.rules[rule_ind];

  if (!this->match_path(rule, od_pair_ind)) {
    return;
  }

  const ChargeTier* matched_tier = nullptr;
  if (!this->match_tier(rule, matched_tier)) {
    return;
  }

  const long amount = this->calc_items(*matched_tier);
  this->accumulate(rule.secondary_ind, od_pair_ind, amount);
}

bool ChargeEvaluator::match_path(const ChargeRule& rule, int& od_pair_ind) {
  if (rule.path_type == ChargePathType::OD_PAIR) {
    if (rule.path_label_slot < 0 || rule.path_values.size() != 2) {
      return false;
    }
    const std::vector<uint64_t>& pairs = tls_od_pairs[rule.path_label_slot];
    const uint64_t key = make_od_key(rule.path_values[0], rule.path_values[1]);
    const auto it = std::ranges::lower_bound(pairs, key);
    if (it == pairs.end() || *it != key) {
      return false;
    }
    const size_t pair_ind = static_cast<size_t>(it - pairs.begin());
    if (pair_ind >= tls_secondary_covered[rule.secondary_ind].size()) {
      return false;
    }
    od_pair_ind = static_cast<int>(pair_ind);
    return true;
  }

  if (rule.path_type == ChargePathType::ORDERED_ROUTE) {
    if (rule.path_label_slot < 0) {
      return false;
    }
    const ChargePathSeq& seq = tls_path_seqs[rule.path_label_slot];
    const int path_stop_count = static_cast<int>(seq.offsets.size()) - 1;
    if (path_stop_count <= 0 || path_stop_count != static_cast<int>(rule.path_values.size())) {
      return false;
    }
    for (int u = 0; u < path_stop_count; u++) {
      if (!slice_contains(seq, u, rule.path_values[u])) {
        return false;
      }
    }
  }
  return true;
}

bool ChargeEvaluator::match_tier(const ChargeRule& rule, const ChargeTier*& tier) {
  long metric = 0;
  switch (rule.condition_type) {
    case ChargeConditionType::NONE:
      break;
    case ChargeConditionType::DISTANCE:
      metric = this->get_distance();
      break;
    case ChargeConditionType::PICK_NODE_COUNT:
      metric = this->get_pick_count();
      break;
    case ChargeConditionType::DROP_NODE_COUNT:
      metric = this->get_drop_count();
      break;
    case ChargeConditionType::DIMENSION: {
      const long* dims = this->get_dim_vals();
      if (dims == nullptr || rule.condition_dim_ind < 0 ||
          rule.condition_dim_ind >= this->manager.dim_len) {
        return false;
      }
      metric = dims[rule.condition_dim_ind];
      break;
    }
  }

  for (int u = 0; u < rule.tier_len; u++) {
    const ChargeTier& cur = this->manager.tiers[rule.tier_begin + u];
    if (metric >= cur.lower && metric < cur.upper) {
      tier = &cur;
      return true;
    }
  }
  return false;
}

long ChargeEvaluator::calc_items(const ChargeTier& tier) {
  long amount = 0;
  for (int u = 0; u < tier.item_len; u++) {
    const ChargeItem& item = this->manager.items[tier.item_begin + u];
    switch (item.type) {
      case ChargeItemType::FLAT:
        amount += item.price;
        break;
      case ChargeItemType::DISTANCE:
        amount += div_round(static_cast<__int128>(item.price) * this->get_distance(),
                            ChargeParameter::METER_PER_KM);
        break;
      case ChargeItemType::PICK_STOP: {
        const int pick_count = this->get_pick_count();
        const int extra = pick_count > 1 ? pick_count - 1 : 0;
        amount += item.price * static_cast<long>(extra);
        break;
      }
      case ChargeItemType::DROP_STOP: {
        const int drop_count = this->get_drop_count();
        const int extra = drop_count > 1 ? drop_count - 1 : 0;
        amount += item.price * static_cast<long>(extra);
        break;
      }
      case ChargeItemType::DIMENSION: {
        const long* dims = this->get_dim_vals();
        if (dims == nullptr || item.dim_ind < 0 || item.dim_ind >= this->manager.dim_len) {
          break;
        }
        amount += div_round(static_cast<__int128>(item.price) * dims[item.dim_ind],
                            this->manager.dim_scales[item.dim_ind]);
        break;
      }
      case ChargeItemType::DIMENSION_DISTANCE: {
        const long* dims = this->get_dim_vals();
        if (dims == nullptr || item.dim_ind < 0 || item.dim_ind >= this->manager.dim_len) {
          break;
        }
        const long denominator =
            this->manager.dim_scales[item.dim_ind] * ChargeParameter::METER_PER_KM;
        amount +=
            div_round(static_cast<__int128>(item.price) * dims[item.dim_ind] * this->get_distance(),
                      denominator);
        break;
      }
    }
  }
  return amount < 0 ? 0 : amount;
}

void ChargeEvaluator::accumulate(const int secondary_ind, const int od_pair_ind,
                                 const long amount) {
  this->touch_secondary(secondary_ind);
  if (od_pair_ind >= 0) {
    std::vector<uint8_t>& covered = tls_secondary_covered[secondary_ind];
    std::vector<long>& pair_sums = tls_secondary_pair_sums[secondary_ind];
    const size_t pair_ind = static_cast<size_t>(od_pair_ind);
    if (pair_ind < covered.size() && pair_ind < pair_sums.size()) {
      covered[pair_ind] = 1;
      pair_sums[pair_ind] += amount;
    }
    return;
  }
  tls_secondary_other_sums[secondary_ind] += amount;
}

ChargeResult ChargeEvaluator::finalize() {
  for (const int secondary_ind : tls_touched_secondaries) {
    long secondary_amount = 0;
    if (this->manager.secondary_od_slot[secondary_ind] >= 0) {
      const int slot = this->manager.secondary_od_slot[secondary_ind];
      if (!tls_od_slot_valid[slot]) {
        continue;
      }
      const std::vector<uint64_t>& pairs = tls_od_pairs[slot];
      const std::vector<uint8_t>& covered = tls_secondary_covered[secondary_ind];
      const std::vector<long>& pair_sums = tls_secondary_pair_sums[secondary_ind];
      if (pairs.empty() || covered.size() != pairs.size() || pair_sums.size() != pairs.size()) {
        continue;
      }
      bool all_covered = true;
      for (const uint8_t flag : covered) {
        if (flag == 0) {
          all_covered = false;
          break;
        }
      }
      if (!all_covered) {
        continue;
      }
      long max_pair_amount = 0;
      for (size_t u = 0; u < pair_sums.size(); u++) {
        if (u == 0 || pair_sums[u] > max_pair_amount) {
          max_pair_amount = pair_sums[u];
        }
      }
      secondary_amount = max_pair_amount + tls_secondary_other_sums[secondary_ind];
    } else {
      secondary_amount = tls_secondary_other_sums[secondary_ind];
    }

    const int charge_code_ind = this->manager.secondary_charge_code_inds[secondary_ind];
    if (tls_charge_code_epoch[charge_code_ind] != tls_epoch) {
      tls_charge_code_epoch[charge_code_ind] = tls_epoch;
      tls_charge_code_max[charge_code_ind] = 0;
      tls_touched_charge_codes.push_back(charge_code_ind);
    }
    if (secondary_amount > tls_charge_code_max[charge_code_ind]) {
      tls_charge_code_max[charge_code_ind] = secondary_amount;
    }
  }

  long best_amount = -1;
  for (const int charge_code_ind : tls_touched_charge_codes) {
    const long amount = tls_charge_code_max[charge_code_ind];
    if (best_amount < 0 || amount < best_amount) {
      best_amount = amount;
    }
  }
  if (best_amount < 0) {
    return ChargeResult{};
  }
  return ChargeResult{best_amount, true};
}

ChargeResult ChargeManager::evaluate(const ChargeLoadView& view) const {
  return ChargeEvaluator(*this, view).run();
}
