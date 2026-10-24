/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#include <algorithm>
#include <limits>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "c_csv_utils.h"
#include "c_file_utils.h"
#include "c_rapidcsv.h"
#include "scr_charge_reader.h"
#include "scr_charge_reader_internal.h"
#include "se_schema.h"

using ChargeReaderInternal::fail;
using ChargeReaderInternal::parse_bound;
using ChargeReaderInternal::parse_item_type;
using ChargeReaderInternal::parse_long;
using ChargeReaderInternal::parse_price;
using ChargeReaderInternal::trim;

void ChargeCsvReader::load_tiers(std::vector<RuleDraft>& rule_drafts,
                                 const std::unordered_map<std::string, int>& rule_index) const {
  this->logger->info("Loading {} ...", ChargeTierSchema::file_name);
  const std::string file_path = this->root_dir + "/" + ChargeTierSchema::file_name;
  FileUtils::file_exists(file_path, ChargeTierSchema::file_name);
  rapidcsv::Document doc(file_path);
  CsvUtils::check_column_exist(doc, ChargeTierSchema::headers, ChargeTierSchema::file_name);

  const size_t row_count = doc.GetRowCount();
  for (size_t u = 0; u < row_count; u++) {
    const std::string rule_code =
        trim(doc.GetCell<std::string>(ChargeTierSchema::headers[ChargeTierSchema::RULE_CODE], u));
    const std::string tier_seq_text =
        trim(doc.GetCell<std::string>(ChargeTierSchema::headers[ChargeTierSchema::TIER_SEQ], u));
    const std::string lower_text =
        trim(doc.GetCell<std::string>(ChargeTierSchema::headers[ChargeTierSchema::LOWER_BOUND], u));
    const std::string upper_text =
        trim(doc.GetCell<std::string>(ChargeTierSchema::headers[ChargeTierSchema::UPPER_BOUND], u));

    const auto rule_it = rule_index.find(rule_code);
    if (rule_it == rule_index.end()) {
      fail(ChargeTierSchema::file_name, u, "unknown RuleCode: " + rule_code);
    }
    RuleDraft& draft = rule_drafts[rule_it->second];
    const int tier_seq =
        static_cast<int>(parse_long(tier_seq_text, ChargeTierSchema::file_name, u, "TierSeq"));
    if (tier_seq <= 0) {
      fail(ChargeTierSchema::file_name, u, "TierSeq must be positive");
    }
    for (const auto& existing_tier : draft.tiers) {
      if (existing_tier.seq == tier_seq) {
        fail(ChargeTierSchema::file_name, u, "duplicate TierSeq: " + tier_seq_text);
      }
    }

    long lower = 0;
    long upper = std::numeric_limits<long>::max();
    if (draft.rule.condition_type != ChargeConditionType::NONE) {
      const Dimension* dimension = draft.rule.condition_dim_ind >= 0
                                       ? this->dim_manager->dimensions[draft.rule.condition_dim_ind]
                                       : nullptr;
      lower = parse_bound(lower_text, draft.rule.condition_type, dimension, false, u);
      upper = parse_bound(upper_text, draft.rule.condition_type, dimension, true, u);
    }
    if (lower >= upper) {
      fail(ChargeTierSchema::file_name, u, "LowerBound must be less than UpperBound");
    }

    draft.tiers.push_back(TierDraft{tier_seq, lower, upper, {}, u});
  }
  doc.Clear();

  for (auto& draft : rule_drafts) {
    if (draft.tiers.empty()) {
      throw std::runtime_error(std::string(ChargeTierSchema::file_name) +
                               ": rule has no tier, RuleCode=" + draft.rule_code);
    }
    if (draft.rule.condition_type == ChargeConditionType::NONE && draft.tiers.size() != 1) {
      throw std::runtime_error(
          std::string(ChargeTierSchema::file_name) +
          ": rule without tier condition must have exactly one tier, RuleCode=" + draft.rule_code);
    }
    if (draft.rule.condition_type != ChargeConditionType::NONE) {
      // 按 lower 排序后检查相邻区间是否重叠，重叠会导致同一 metric 命中多个阶梯
      std::ranges::sort(draft.tiers,
                        [](const TierDraft& a, const TierDraft& b) { return a.lower < b.lower; });
      for (size_t tier_ind = 1; tier_ind < draft.tiers.size(); tier_ind++) {
        const TierDraft& prev = draft.tiers[tier_ind - 1];
        const TierDraft& cur = draft.tiers[tier_ind];
        if (prev.upper > cur.lower) {
          throw std::runtime_error(
              std::string(ChargeTierSchema::file_name) + " line " + std::to_string(prev.row + 2) +
              " and line " + std::to_string(cur.row + 2) +
              ": tier ranges overlap, RuleCode=" + draft.rule_code +
              ", TierSeq=" + std::to_string(prev.seq) + "/" + std::to_string(cur.seq));
        }
      }
    }
    std::ranges::sort(draft.tiers,
                      [](const TierDraft& a, const TierDraft& b) { return a.seq < b.seq; });
  }
  this->logger->info("Loading {} is complete. A total of {} pieces of data have been obtained.",
                     ChargeTierSchema::file_name, row_count);
}

void ChargeCsvReader::load_items(std::vector<RuleDraft>& rule_drafts,
                                 const std::unordered_map<std::string, int>& rule_index) const {
  this->logger->info("Loading {} ...", ChargeItemSchema::file_name);
  const std::string file_path = this->root_dir + "/" + ChargeItemSchema::file_name;
  FileUtils::file_exists(file_path, ChargeItemSchema::file_name);
  rapidcsv::Document doc(file_path);
  CsvUtils::check_column_exist(doc, ChargeItemSchema::headers, ChargeItemSchema::file_name);

  const size_t row_count = doc.GetRowCount();
  for (size_t u = 0; u < row_count; u++) {
    const std::string rule_code =
        trim(doc.GetCell<std::string>(ChargeItemSchema::headers[ChargeItemSchema::RULE_CODE], u));
    const std::string tier_seq_text =
        trim(doc.GetCell<std::string>(ChargeItemSchema::headers[ChargeItemSchema::TIER_SEQ], u));
    const std::string item_type_text =
        trim(doc.GetCell<std::string>(ChargeItemSchema::headers[ChargeItemSchema::ITEM_TYPE], u));
    const std::string price_text =
        trim(doc.GetCell<std::string>(ChargeItemSchema::headers[ChargeItemSchema::PRICE], u));
    const std::string dimension_code = trim(
        doc.GetCell<std::string>(ChargeItemSchema::headers[ChargeItemSchema::DIMENSION_CODE], u));

    const auto rule_it = rule_index.find(rule_code);
    if (rule_it == rule_index.end()) {
      fail(ChargeItemSchema::file_name, u, "unknown RuleCode: " + rule_code);
    }
    RuleDraft& draft = rule_drafts[rule_it->second];
    const int tier_seq =
        static_cast<int>(parse_long(tier_seq_text, ChargeItemSchema::file_name, u, "TierSeq"));
    TierDraft* tier = nullptr;
    for (auto& candidate : draft.tiers) {
      if (candidate.seq == tier_seq) {
        tier = &candidate;
        break;
      }
    }
    if (tier == nullptr) {
      fail(ChargeItemSchema::file_name, u, "unknown TierSeq: " + tier_seq_text);
    }

    const ChargeItemType item_type = parse_item_type(item_type_text, u);
    const long price = parse_price(price_text, u);
    int dim_ind = -1;
    if (item_type == ChargeItemType::DIMENSION || item_type == ChargeItemType::DIMENSION_DISTANCE) {
      if (dimension_code.empty()) {
        fail(ChargeItemSchema::file_name, u, "DimensionCode is required for dimensional item");
      }
      const Dimension* dimension = this->dim_manager->get_dimension(dimension_code);
      if (dimension == nullptr) {
        fail(ChargeItemSchema::file_name, u, "unknown DimensionCode: " + dimension_code);
      }
      dim_ind = dimension->ind;
    } else if (!dimension_code.empty()) {
      fail(ChargeItemSchema::file_name, u, "DimensionCode is only allowed for dimensional item");
    }

    tier->items.push_back(ChargeItem{item_type, price, dim_ind});
  }
  doc.Clear();

  for (const auto& draft : rule_drafts) {
    for (const auto& tier : draft.tiers) {
      if (tier.items.empty()) {
        throw std::runtime_error(std::string(ChargeItemSchema::file_name) +
                                 ": tier has no item, RuleCode=" + draft.rule_code +
                                 ", TierSeq=" + std::to_string(tier.seq));
      }
    }
  }
  this->logger->info("Loading {} is complete. A total of {} pieces of data have been obtained.",
                     ChargeItemSchema::file_name, row_count);
}

void ChargeCsvReader::finalize(ChargeManager* manager, std::vector<RuleDraft>& rule_drafts) const {
  manager->rules.reserve(rule_drafts.size());
  for (auto& draft : rule_drafts) {
    // 阶梯重叠已在 load_tiers 中校验，这里只按 lower 排序以便拍平
    std::ranges::sort(draft.tiers,
                      [](const TierDraft& a, const TierDraft& b) { return a.lower < b.lower; });

    draft.rule.tier_begin = static_cast<int>(manager->tiers.size());
    for (auto& tier_draft : draft.tiers) {
      ChargeTier tier{};
      tier.lower = tier_draft.lower;
      tier.upper = tier_draft.upper;
      tier.item_begin = static_cast<int>(manager->items.size());
      tier.item_len = static_cast<int>(tier_draft.items.size());
      manager->items.insert(manager->items.end(), tier_draft.items.begin(), tier_draft.items.end());
      manager->tiers.push_back(tier);
    }
    draft.rule.tier_len = static_cast<int>(manager->tiers.size()) - draft.rule.tier_begin;
    manager->rules.push_back(std::move(draft.rule));
  }

  manager->build_indexes(this->dim_manager, this->label_manager, this->carrier_manager,
                         this->vehicle_model_manager, this->location_manager);
  this->logger->info("Charge loaded: {} charge codes, {} rules, {} tiers, {} items, {} vehicles.",
                     manager->charge_code_codes.size(), manager->rules.size(),
                     manager->tiers.size(), manager->items.size(),
                     this->carrier_manager->vehicles.size());
}
