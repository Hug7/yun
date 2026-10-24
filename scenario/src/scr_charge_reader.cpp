/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#include "scr_charge_reader.h"

#include <string>
#include <unordered_set>
#include <utility>

#include "c_csv_utils.h"
#include "c_file_utils.h"
#include "c_rapidcsv.h"
#include "scr_charge_reader_internal.h"
#include "se_schema.h"

using ChargeReaderInternal::fail;
using ChargeReaderInternal::parse_condition_type;
using ChargeReaderInternal::parse_path_type;
using ChargeReaderInternal::split_path_values;
using ChargeReaderInternal::trim;

// ====== implement of ChargeCsvReader ======
ChargeCsvReader::ChargeCsvReader(std::string root_dir, std::shared_ptr<spdlog::logger> logger,
                                 DimensionManager* dim_manager, LabelManager* label_manager,
                                 CarrierManager* carrier_manager,
                                 VehicleModelManager* vehicle_model_manager,
                                 LocationManager* location_manager)
    : root_dir(std::move(root_dir)),
      logger(std::move(logger)),
      dim_manager(dim_manager),
      label_manager(label_manager),
      carrier_manager(carrier_manager),
      vehicle_model_manager(vehicle_model_manager),
      location_manager(location_manager) {}

ChargeManager* ChargeCsvReader::load() const {
  auto manager = std::make_unique<ChargeManager>();
  manager->dim_len = this->dim_manager->len;

  const std::string rule_file_path = this->root_dir + "/" + ChargeRuleSchema::file_name;
  if (!FileUtils::file_exists(rule_file_path)) {
    this->logger->info("{} not found, charge is disabled.", ChargeRuleSchema::file_name);
    return manager.release();
  }

  std::unordered_map<std::string, int> charge_code_index;
  this->load_charge_codes(manager.get(), charge_code_index);

  std::unordered_map<std::string, int> rule_index;
  std::vector<RuleDraft> rule_drafts;
  this->load_rules(manager.get(), charge_code_index, rule_index, rule_drafts);
  if (rule_drafts.empty()) {
    this->logger->info("{} has no rule, charge is disabled.", ChargeRuleSchema::file_name);
    return manager.release();
  }

  this->load_tiers(rule_drafts, rule_index);
  this->load_items(rule_drafts, rule_index);
  this->finalize(manager.get(), rule_drafts);

  return manager.release();
}

void ChargeCsvReader::load_charge_codes(
    ChargeManager* manager, std::unordered_map<std::string, int>& charge_code_index) const {
  this->logger->info("Loading {} ...", ChargeCodeSchema::file_name);
  const std::string file_path = this->root_dir + "/" + ChargeCodeSchema::file_name;
  FileUtils::file_exists(file_path, ChargeCodeSchema::file_name);
  rapidcsv::Document doc(file_path);
  CsvUtils::check_column_exist(doc, ChargeCodeSchema::headers, ChargeCodeSchema::file_name);

  const size_t row_count = doc.GetRowCount();
  for (size_t u = 0; u < row_count; u++) {
    const std::string code =
        trim(doc.GetCell<std::string>(ChargeCodeSchema::headers[ChargeCodeSchema::CODE], u));
    const std::string name =
        trim(doc.GetCell<std::string>(ChargeCodeSchema::headers[ChargeCodeSchema::NAME], u));
    if (code.empty()) {
      fail(ChargeCodeSchema::file_name, u, "Code must not be empty");
    }
    const int charge_code_ind = static_cast<int>(manager->charge_code_codes.size());
    if (!charge_code_index.emplace(code, charge_code_ind).second) {
      fail(ChargeCodeSchema::file_name, u, "duplicate Code: " + code);
    }
    manager->charge_code_codes.push_back(code);
    manager->charge_code_names.push_back(name);
  }
  doc.Clear();
  this->logger->info("Loading {} is complete. A total of {} pieces of data have been obtained.",
                     ChargeCodeSchema::file_name, row_count);
}

void ChargeCsvReader::load_rules(ChargeManager* charge_manager,
                                 const std::unordered_map<std::string, int>& charge_code_index,
                                 std::unordered_map<std::string, int>& rule_index,
                                 std::vector<RuleDraft>& rule_drafts) const {
  this->logger->info("Loading {} ...", ChargeRuleSchema::file_name);
  const std::string file_path = this->root_dir + "/" + ChargeRuleSchema::file_name;
  FileUtils::file_exists(file_path, ChargeRuleSchema::file_name);
  rapidcsv::Document doc(file_path);
  CsvUtils::check_column_exist(doc, ChargeRuleSchema::headers, ChargeRuleSchema::file_name);

  std::unordered_set<std::string> match_keys;
  const size_t row_count = doc.GetRowCount();
  for (size_t u = 0; u < row_count; u++) {
    const std::string rule_code =
        trim(doc.GetCell<std::string>(ChargeRuleSchema::headers[ChargeRuleSchema::RULE_CODE], u));
    const std::string charge_code =
        trim(doc.GetCell<std::string>(ChargeRuleSchema::headers[ChargeRuleSchema::CHARGE_CODE], u));
    const std::string secondary_code = trim(doc.GetCell<std::string>(
        ChargeRuleSchema::headers[ChargeRuleSchema::CHARGE_SECONDARY_CODE], u));
    const std::string carrier_code = trim(
        doc.GetCell<std::string>(ChargeRuleSchema::headers[ChargeRuleSchema::CARRIER_CODE], u));
    const std::string vehicle_model_code = trim(doc.GetCell<std::string>(
        ChargeRuleSchema::headers[ChargeRuleSchema::VEHICLE_MODEL_CODE], u));
    const std::string path_type_text = trim(
        doc.GetCell<std::string>(ChargeRuleSchema::headers[ChargeRuleSchema::CHARGE_PATH_TYPE], u));
    const std::string label_code = trim(doc.GetCell<std::string>(
        ChargeRuleSchema::headers[ChargeRuleSchema::LOCATION_LABEL_CODE], u));
    const std::string path_values_text = trim(doc.GetCell<std::string>(
        ChargeRuleSchema::headers[ChargeRuleSchema::CHARGE_PATH_LABEL_VALUES], u));
    const std::string condition_type_text = trim(doc.GetCell<std::string>(
        ChargeRuleSchema::headers[ChargeRuleSchema::TIER_CONDITION_TYPE], u));
    const std::string dimension_code = trim(doc.GetCell<std::string>(
        ChargeRuleSchema::headers[ChargeRuleSchema::TIER_DIMENSION_CODE], u));

    if (rule_code.empty()) {
      fail(ChargeRuleSchema::file_name, u, "RuleCode must not be empty");
    }
    if (rule_index.contains(rule_code)) {
      fail(ChargeRuleSchema::file_name, u, "duplicate RuleCode: " + rule_code);
    }
    if (secondary_code.empty()) {
      fail(ChargeRuleSchema::file_name, u, "ChargeSecondaryCode must not be empty");
    }

    RuleDraft draft;
    draft.rule_code = rule_code;
    ChargeRule& rule = draft.rule;

    const auto charge_code_it = charge_code_index.find(charge_code);
    if (charge_code_it == charge_code_index.end()) {
      fail(ChargeRuleSchema::file_name, u, "unknown ChargeCode: " + charge_code);
    }
    rule.charge_code_ind = charge_code_it->second;
    rule.secondary_ind = charge_manager->add_secondary(rule.charge_code_ind, secondary_code);

    if (!carrier_code.empty()) {
      Carrier* carrier = this->carrier_manager->get_carrier(carrier_code);
      if (carrier == nullptr) {
        fail(ChargeRuleSchema::file_name, u, "unknown CarrierCode: " + carrier_code);
      }
      rule.carrier_ind = carrier->ind;
    }
    if (!vehicle_model_code.empty()) {
      VehicleModel* vehicle_model =
          this->vehicle_model_manager->get_vehicle_model(vehicle_model_code);
      if (vehicle_model == nullptr) {
        fail(ChargeRuleSchema::file_name, u, "unknown VehicleModelCode: " + vehicle_model_code);
      }
      rule.vehicle_model_ind = vehicle_model->ind;
    }

    rule.path_type = parse_path_type(path_type_text, u);
    if (rule.path_type == ChargePathType::NONE) {
      if (!label_code.empty() || !path_values_text.empty()) {
        fail(ChargeRuleSchema::file_name, u,
             "LocationLabelCode/ChargePathLabelValues require ChargePathType");
      }
    } else {
      if (label_code.empty() || path_values_text.empty()) {
        fail(ChargeRuleSchema::file_name, u,
             "LocationLabelCode and ChargePathLabelValues are required");
      }
      const int label_ind = this->label_manager->location_labelset->get_label_ind(label_code);
      if (label_ind < 0) {
        fail(ChargeRuleSchema::file_name, u, "unknown LocationLabelCode: " + label_code);
      }
      Label* label = this->label_manager->location_labelset->get_label(label_code);
      const std::vector<std::string> tokens = split_path_values(path_values_text);
      if (tokens.empty()) {
        fail(ChargeRuleSchema::file_name, u, "ChargePathLabelValues must not be empty");
      }
      if (rule.path_type == ChargePathType::OD_PAIR && tokens.size() != 2) {
        fail(ChargeRuleSchema::file_name, u, "OD_PAIR requires exactly two label values");
      }
      rule.path_label_ind4labelset = label_ind;
      rule.path_values.reserve(tokens.size());
      for (const auto& token : tokens) {
        LabelValue* label_value = label->get_label_value(token);
        if (label_value == nullptr) {
          fail(ChargeRuleSchema::file_name, u,
               "unknown label value " + token + " for label " + label_code);
        }
        rule.path_values.push_back(label_value->ind);
      }
    }

    rule.condition_type = parse_condition_type(condition_type_text, u);
    if (rule.condition_type == ChargeConditionType::DIMENSION) {
      if (dimension_code.empty()) {
        fail(ChargeRuleSchema::file_name, u,
             "TierDimensionCode is required when TierConditionType=DIMENSION");
      }
      Dimension* dimension = this->dim_manager->get_dimension(dimension_code);
      if (dimension == nullptr) {
        fail(ChargeRuleSchema::file_name, u, "unknown TierDimensionCode: " + dimension_code);
      }
      rule.condition_dim_ind = dimension->ind;
    } else if (!dimension_code.empty()) {
      fail(ChargeRuleSchema::file_name, u,
           "TierDimensionCode is only allowed when TierConditionType=DIMENSION");
    }

    std::string match_key;
    match_key.append(std::to_string(rule.charge_code_ind)).push_back('|');
    match_key.append(std::to_string(rule.secondary_ind)).push_back('|');
    match_key.append(std::to_string(rule.carrier_ind)).push_back('|');
    match_key.append(std::to_string(rule.vehicle_model_ind)).push_back('|');
    match_key.append(std::to_string(static_cast<int>(rule.path_type))).push_back('|');
    match_key.append(std::to_string(rule.path_label_ind4labelset)).push_back('|');
    for (const int value : rule.path_values) {
      match_key.append(std::to_string(value)).push_back(',');
    }
    match_key.push_back('|');
    match_key.append(std::to_string(static_cast<int>(rule.condition_type))).push_back('|');
    match_key.append(std::to_string(rule.condition_dim_ind));
    if (!match_keys.insert(match_key).second) {
      fail(ChargeRuleSchema::file_name, u, "duplicate rule query key: " + rule_code);
    }

    rule_index[rule_code] = static_cast<int>(rule_drafts.size());
    rule_drafts.push_back(std::move(draft));
  }
  doc.Clear();
  this->logger->info("Loading {} is complete. A total of {} pieces of data have been obtained.",
                     ChargeRuleSchema::file_name, row_count);
}
