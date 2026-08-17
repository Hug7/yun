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

LocationManager* StandardCsvReader::loading_location(LabelManager* label_manager) {
  LocationManager* location_manager = new LocationManager(label_manager->location_labelset);
  // === loading Location.csv ===
  spdlog::info("Loading {} ...", LocationSchema::file_name);
  const std::string location_file_name =
      this->root_dir + "/" + std::string(LocationSchema::file_name);
  FileUtils::file_exists(location_file_name, LocationSchema::file_name);
  rapidcsv::Document location_doc(location_file_name);
  CsvUtils::check_column_exist(location_doc, LocationSchema::headers, LocationSchema::file_name);

  int location_count = location_doc.GetRowCount();
  for (int u = 0; u < location_count; u++) {
    const std::string location_code =
        location_doc.GetCell<std::string>(LocationSchema::headers[LocationSchema::CODE], u);
    const std::string location_name =
        location_doc.GetCell<std::string>(LocationSchema::headers[LocationSchema::NAME], u);
    const double lng =
        location_doc.GetCell<double>(LocationSchema::headers[LocationSchema::LNG], u);
    const double lat =
        location_doc.GetCell<double>(LocationSchema::headers[LocationSchema::LAT], u);

    if (location_manager->get_location(location_code) != nullptr) {
      throw std::runtime_error(std::string(LocationSchema::file_name) + " location code " +
                               location_code + " is repetitive");
    }
    location_manager->create_location(location_code, location_name, lat, lng);
  }
  location_doc.Clear();
  spdlog::info("Loading {} is complete. A total of {} pieces of data have been obtained.",
               LocationSchema::file_name, location_count);

  // === loading LocationLabelValue.csv ===
  spdlog::info("Loading {} ...", LocationLabelValueSchema::file_name);
  const std::string location_label_value_file_name =
      this->root_dir + "/" + std::string(LocationLabelValueSchema::file_name);
  FileUtils::file_exists(location_label_value_file_name, LocationLabelValueSchema::file_name);
  rapidcsv::Document location_label_value_doc(location_label_value_file_name);
  CsvUtils::check_column_exist(location_label_value_doc, LocationLabelValueSchema::headers,
                               LocationLabelValueSchema::file_name);

  int location_label_value_count = location_label_value_doc.GetRowCount();
  for (int u = 0; u < location_label_value_count; u++) {
    const std::string location_code = location_label_value_doc.GetCell<std::string>(
        LocationLabelValueSchema::headers[LocationLabelValueSchema::LOCATION_CODE], u);
    const std::string label_code = location_label_value_doc.GetCell<std::string>(
        LocationLabelValueSchema::headers[LocationLabelValueSchema::LABEL_CODE], u);
    const std::string label_value = location_label_value_doc.GetCell<std::string>(
        LocationLabelValueSchema::headers[LocationLabelValueSchema::LABEL_VALUE], u);

    Location* location = location_manager->get_location(location_code);
    if (location == nullptr) {
      throw std::runtime_error(std::string(LocationLabelValueSchema::file_name) +
                               " location code " + location_code + " not exist");
    }
    Label* label = label_manager->get_label(label_code);
    if (label == nullptr) {
      throw std::runtime_error(std::string(LocationLabelValueSchema::file_name) + " label code " +
                               label_code + " not exist");
    }
    LabelValue* label_value_ptr = label->get_label_value(label_value);
    if (label_value_ptr == nullptr) {
      throw std::runtime_error("Label value " + label_value + " was not found for Label " +
                               label_code + " in " +
                               std::string(LocationLabelValueSchema::file_name));
    }
    int label_ind4labelset = location_manager->labelset->get_label_ind(label_code);
    if (label_ind4labelset == -1) {
      throw std::runtime_error("Label " + label_code + " was not found in labelset of location" +
                               location_code);
    }

    location->add_label_value(label_ind4labelset, label_value_ptr);
  }
  location_label_value_doc.Clear();
  spdlog::info("Loading {} is complete. A total of {} pieces of data have been obtained.",
               LocationLabelValueSchema::file_name, location_label_value_count);

  return location_manager;
}
