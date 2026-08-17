/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string>

#include "c_csv_utils.h"
#include "c_file_utils.h"
#include "c_rapidcsv.h"
#include "se_schema.h"
#include "standard_csv_reader.h"

CarrierManager* StandardCsvReader::loading_carrier(LabelManager* label_manager) {
  CarrierManager* carrier_manager = new CarrierManager(label_manager->carrier_labelset);

  // === loading Carrier.csv ===
  spdlog::info("Loading {} ...", CarrierSchema::file_name);
  const std::string carrier_file_path = this->root_dir + "/" + CarrierSchema::file_name;
  FileUtils::file_exists(carrier_file_path, CarrierSchema::file_name);
  rapidcsv::Document carrier_doc(carrier_file_path);
  CsvUtils::check_column_exist(carrier_doc, CarrierSchema::headers, CarrierSchema::file_name);

  int carrier_count = carrier_doc.GetRowCount();
  for (int u = 0; u < carrier_count; u++) {
    const std::string carrier_code =
        carrier_doc.GetCell<std::string>(CarrierSchema::headers[CarrierSchema::CODE], u);
    const std::string carrier_name =
        carrier_doc.GetCell<std::string>(CarrierSchema::headers[CarrierSchema::NAME], u);

    carrier_manager->create_carrier(carrier_code, carrier_name);
  }
  carrier_doc.Clear();
  spdlog::info("Loading {} is complete. A total of {} pieces of data have been obtained.",
               CarrierSchema::file_name, carrier_count);

  // === loading CarrierLabelValue.csv ===
  spdlog::info("Loading {} ...", CarrierLabelValueSchema::file_name);
  const std::string carrier_label_value_file_path =
      this->root_dir + "/" + CarrierLabelValueSchema::file_name;
  FileUtils::file_exists(carrier_label_value_file_path, CarrierLabelValueSchema::file_name);
  rapidcsv::Document carrier_label_value_doc(carrier_label_value_file_path);
  CsvUtils::check_column_exist(carrier_label_value_doc, CarrierLabelValueSchema::headers,
                               CarrierLabelValueSchema::file_name);

  int carrier_label_value_count = carrier_label_value_doc.GetRowCount();
  for (int u = 0; u < carrier_label_value_count; u++) {
    const std::string carrier_code = carrier_doc.GetCell<std::string>(
        CarrierLabelValueSchema::headers[CarrierLabelValueSchema::CARRIER_CODE], u);
    const std::string label_code = carrier_doc.GetCell<std::string>(
        CarrierLabelValueSchema::headers[CarrierLabelValueSchema::LABEL_CODE], u);
    const std::string label_value = carrier_doc.GetCell<std::string>(
        CarrierLabelValueSchema::headers[CarrierLabelValueSchema::LABEL_VALUE], u);

    Carrier* carrier = carrier_manager->get_carrier(carrier_code);
    if (carrier == nullptr) {
      throw std::runtime_error("Carrier " + carrier_code + " was not found in " +
                               CarrierLabelValueSchema::file_name);
    }

    Label* label = label_manager->get_label(label_code);
    if (label == nullptr) {
      throw std::runtime_error("Label " + label_code + " was not found in " +
                               CarrierLabelValueSchema::file_name);
    }

    LabelValue* label_value_ptr = label->get_label_value(label_value);
    if (label_value_ptr == nullptr) {
      throw std::runtime_error("Label value " + label_value + " was not found for Label " +
                               label_code + " in " + CarrierLabelValueSchema::file_name);
    }

    int label_ind4labelset = carrier_manager->labelset->get_label_ind(label_code);
    if (label_ind4labelset == -1) {
      throw std::runtime_error("Label " + label_code + " was not found in labelset of carrier" +
                               carrier_code);
    }

    carrier->update_labelset_value(label_ind4labelset, label_value_ptr);
  }
  carrier_label_value_doc.Clear();
  spdlog::info("Loading {} is complete. A total of {} pieces of data have been obtained.",
               CarrierLabelValueSchema::file_name, carrier_label_value_count);

  return carrier_manager;
}
