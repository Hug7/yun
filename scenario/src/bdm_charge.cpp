/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#include "bdm_charge.h"

#include <algorithm>
#include <stdexcept>
#include <string>

// ====== implement of ChargeManager ======
int ChargeManager::add_secondary(const int charge_code_ind, const std::string& secondary_code) {
  const int secondary_count = static_cast<int>(this->secondary_codes.size());
  for (int u = 0; u < secondary_count; u++) {
    if (this->secondary_charge_code_inds[u] == charge_code_ind &&
        this->secondary_codes[u] == secondary_code) {
      return u;
    }
  }
  this->secondary_charge_code_inds.push_back(charge_code_ind);
  this->secondary_codes.push_back(secondary_code);
  return static_cast<int>(this->secondary_codes.size()) - 1;
}

void ChargeManager::build_indexes(const DimensionManager* dim_manager,
                                  const LabelManager* label_manager,
                                  const CarrierManager* carrier_manager,
                                  const VehicleModelManager* vehicle_model_manager,
                                  const LocationManager* location_manager) {
  // === 校验并建立维度精度表 ===
  this->dim_len = dim_manager->len;
  this->dim_scales.assign(this->dim_len, 1);
  for (int u = 0; u < this->dim_len; u++) {
    const int precision = dim_manager->dimensions[u]->precision;
    if (precision < 0 || precision > 9) {
      throw std::runtime_error("ChargeManager: dimension " + dim_manager->dimensions[u]->code +
                               " precision must be in [0, 9]");
    }
    long scale = 1;
    for (int p = 0; p < precision; p++) {
      scale *= 10;
    }
    this->dim_scales[u] = scale;
  }

  // === 校验规则中的整数索引 ===
  const int charge_code_count = static_cast<int>(this->charge_code_codes.size());
  const int secondary_count = static_cast<int>(this->secondary_codes.size());
  for (const int charge_code_ind : this->secondary_charge_code_inds) {
    if (charge_code_ind < 0 || charge_code_ind >= charge_code_count) {
      throw std::runtime_error("ChargeManager: invalid charge code index in secondary");
    }
  }
  for (const auto& rule : this->rules) {
    if (rule.charge_code_ind < 0 || rule.charge_code_ind >= charge_code_count) {
      throw std::runtime_error("ChargeManager: invalid charge code index in rule");
    }
    if (rule.secondary_ind < 0 || rule.secondary_ind >= secondary_count) {
      throw std::runtime_error("ChargeManager: invalid secondary code index in rule");
    }
    if (rule.carrier_ind >= static_cast<int>(carrier_manager->carriers.size())) {
      throw std::runtime_error("ChargeManager: invalid carrier index in rule");
    }
    if (rule.vehicle_model_ind >= vehicle_model_manager->len) {
      throw std::runtime_error("ChargeManager: invalid vehicle model index in rule");
    }
    if (rule.condition_dim_ind >= this->dim_len) {
      throw std::runtime_error("ChargeManager: invalid dimension index in rule");
    }
    if (rule.tier_begin < 0 || rule.tier_len <= 0 ||
        rule.tier_begin + rule.tier_len > static_cast<int>(this->tiers.size())) {
      throw std::runtime_error("ChargeManager: invalid tier range in rule");
    }
    if (rule.path_type == ChargePathType::OD_PAIR && rule.path_values.size() != 2) {
      throw std::runtime_error("ChargeManager: OD_PAIR requires exactly two path values");
    }
    if (rule.path_type == ChargePathType::ORDERED_ROUTE && rule.path_values.empty()) {
      throw std::runtime_error("ChargeManager: ORDERED_ROUTE requires path values");
    }
  }
  for (const auto& tier : this->tiers) {
    if (tier.item_begin < 0 || tier.item_len <= 0 ||
        tier.item_begin + tier.item_len > static_cast<int>(this->items.size())) {
      throw std::runtime_error("ChargeManager: invalid item range in tier");
    }
  }
  for (const auto& item : this->items) {
    if (item.dim_ind >= this->dim_len) {
      throw std::runtime_error("ChargeManager: invalid dimension index in item");
    }
  }

  // === 线路标签槽位 ===
  this->path_label_inds.clear();
  this->path_label_count = 0;
  const int label_count = label_manager->location_labelset->len;
  std::vector<int> slot_by_label(label_count, -1);
  for (auto& rule : this->rules) {
    if (rule.path_type == ChargePathType::NONE) {
      rule.path_label_slot = -1;
      continue;
    }
    const int label_ind = rule.path_label_ind4labelset;
    if (label_ind < 0 || label_ind >= label_count) {
      throw std::runtime_error("ChargeManager: invalid location label index in rule");
    }
    int slot = slot_by_label[label_ind];
    if (slot < 0) {
      slot = this->path_label_count++;
      slot_by_label[label_ind] = slot;
      this->path_label_inds.push_back(label_ind);
    }
    rule.path_label_slot = slot;
  }

  this->od_path_label_slots.clear();
  this->ordered_path_label_slots.clear();
  this->secondary_od_slot.assign(secondary_count, -1);
  this->od_secondary_inds.clear();
  if (this->path_label_count > 0) {
    std::vector<char> is_od(this->path_label_count, 0);
    std::vector<char> is_ordered(this->path_label_count, 0);
    for (const auto& rule : this->rules) {
      if (rule.path_type == ChargePathType::OD_PAIR && rule.path_label_slot >= 0) {
        is_od[rule.path_label_slot] = 1;
      } else if (rule.path_type == ChargePathType::ORDERED_ROUTE && rule.path_label_slot >= 0) {
        is_ordered[rule.path_label_slot] = 1;
      }
    }
    for (int slot = 0; slot < this->path_label_count; slot++) {
      if (is_od[slot]) {
        this->od_path_label_slots.push_back(slot);
      }
      if (is_ordered[slot]) {
        this->ordered_path_label_slots.push_back(slot);
      }
    }
  }

  // === 每个次级编码的 OD 标签槽位 ===
  for (const auto& rule : this->rules) {
    if (rule.path_type != ChargePathType::OD_PAIR) {
      continue;
    }
    const int slot = rule.path_label_slot;
    if (slot < 0) {
      throw std::runtime_error("ChargeManager: OD rule has no path label slot");
    }
    const int secondary_ind = rule.secondary_ind;
    if (this->secondary_od_slot[secondary_ind] < 0) {
      this->secondary_od_slot[secondary_ind] = slot;
    } else if (this->secondary_od_slot[secondary_ind] != slot) {
      throw std::runtime_error(
          "ChargeManager: OD rules under the same ChargeCode+ChargeSecondaryCode must use the "
          "same LocationLabelCode");
    }
  }
  for (int secondary_ind = 0; secondary_ind < secondary_count; secondary_ind++) {
    if (this->secondary_od_slot[secondary_ind] >= 0) {
      this->od_secondary_inds.push_back(secondary_ind);
    }
  }

  // === 站点标签值 CSR ===
  this->location_count = location_manager->len;
  if (static_cast<int>(location_manager->locations.size()) != this->location_count) {
    throw std::runtime_error("ChargeManager: inconsistent location count");
  }
  this->loc_label_offsets.clear();
  this->loc_label_values.clear();
  if (this->path_label_count > 0) {
    const int location_count = this->location_count;
    const int path_label_count = this->path_label_count;
    this->loc_label_offsets.assign(static_cast<size_t>(location_count) * path_label_count + 1, 0);
    int cursor = 0;
    for (int location_ind = 0; location_ind < location_count; location_ind++) {
      const Location* location = location_manager->locations[location_ind];
      if (location->ind != location_ind) {
        throw std::runtime_error("ChargeManager: location index is not contiguous");
      }
      for (int slot = 0; slot < path_label_count; slot++) {
        const int base = location_ind * path_label_count + slot;
        this->loc_label_offsets[base] = cursor;
        std::vector<int> values;
        const int label_ind = this->path_label_inds[slot];
        if (label_ind < static_cast<int>(location->labelset_value->label_values.size())) {
          const auto& value_map = location->labelset_value->label_values[label_ind];
          values.reserve(value_map.size());
          for (const auto& label_value : value_map) {
            values.push_back(label_value.first);
          }
          std::ranges::sort(values);
        }
        this->loc_label_values.insert(this->loc_label_values.end(), values.begin(), values.end());
        cursor += static_cast<int>(values.size());
      }
    }
    this->loc_label_offsets[static_cast<size_t>(location_count) * path_label_count] = cursor;
  }

  // === 车辆规则索引 ===
  this->generic_rule_inds.clear();
  std::vector<int> specific_rule_inds;
  const int rule_count = static_cast<int>(this->rules.size());
  for (int rule_ind = 0; rule_ind < rule_count; rule_ind++) {
    const ChargeRule& rule = this->rules[rule_ind];
    if (rule.carrier_ind < 0 && rule.vehicle_model_ind < 0) {
      this->generic_rule_inds.push_back(rule_ind);
    } else {
      specific_rule_inds.push_back(rule_ind);
    }
  }
  const int vehicle_count = static_cast<int>(carrier_manager->vehicles.size());
  this->vehicle_rule_offsets.assign(static_cast<size_t>(vehicle_count) + 1, 0);
  this->vehicle_rule_inds.clear();
  int vehicle_rule_cursor = 0;
  for (int vehicle_ind = 0; vehicle_ind < vehicle_count; vehicle_ind++) {
    const Vehicle* vehicle = carrier_manager->vehicles[vehicle_ind];
    if (vehicle->ind != vehicle_ind) {
      throw std::runtime_error("ChargeManager: vehicle index is not contiguous");
    }
    this->vehicle_rule_offsets[vehicle_ind] = vehicle_rule_cursor;
    for (const int rule_ind : specific_rule_inds) {
      const ChargeRule& rule = this->rules[rule_ind];
      if ((rule.carrier_ind < 0 || rule.carrier_ind == vehicle->carrier->ind) &&
          (rule.vehicle_model_ind < 0 || rule.vehicle_model_ind == vehicle->vehicle_model->ind)) {
        this->vehicle_rule_inds.push_back(rule_ind);
        vehicle_rule_cursor++;
      }
    }
  }
  this->vehicle_rule_offsets[vehicle_count] = vehicle_rule_cursor;

  this->enabled = !this->rules.empty();
}
