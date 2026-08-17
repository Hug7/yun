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

DistMatrixManager* StandardCsvReader::loading_dist_matrix(LocationManager* location_manager) {
  DistMatrixManager* dist_matrix_manager = new DistMatrixManager(location_manager->len);
  // === loading DistMatrix.csv ===
  spdlog::info("Loading {} ...", DistMatrixCodeSchema::file_name);
  const std::string dist_matrix_code_file_path =
      this->root_dir + "/" + std::string(DistMatrixCodeSchema::file_name);
  FileUtils::file_exists(dist_matrix_code_file_path, DistMatrixCodeSchema::file_name);
  rapidcsv::Document dist_matrix_code_doc(dist_matrix_code_file_path);
  CsvUtils::check_column_exist(dist_matrix_code_doc, DistMatrixCodeSchema::headers,
                               DistMatrixCodeSchema::file_name);

  int dist_matrix_code_count = dist_matrix_code_doc.GetRowCount();
  for (int u = 0; u < dist_matrix_code_count; u++) {
    const std::string code = dist_matrix_code_doc.GetCell<std::string>(
        DistMatrixCodeSchema::headers[DistMatrixCodeSchema::CODE], u);
    const std::string name = dist_matrix_code_doc.GetCell<std::string>(
        DistMatrixCodeSchema::headers[DistMatrixCodeSchema::NAME], u);

    dist_matrix_manager->create_dist_matrix_code(code, name);
  }
  dist_matrix_code_doc.Clear();
  spdlog::info("Loading {} is complete. A total of {} pieces of data have been obtained.",
               DistMatrixCodeSchema::file_name, dist_matrix_code_count);

  // === loading DistMatrix.csv ===
  spdlog::info("Loading {} ...", DistMatrixSchema::file_name);
  std::string dist_matrix_file_path = this->root_dir + "/" + DistMatrixSchema::file_name;
  FileUtils::file_exists(dist_matrix_file_path, DistMatrixSchema::file_name);
  rapidcsv::Document dist_matrix_doc(dist_matrix_file_path);
  CsvUtils::check_column_exist(dist_matrix_doc, DistMatrixSchema::headers,
                               DistMatrixSchema::file_name);

  int dist_matrix_count = dist_matrix_doc.GetRowCount();
  for (int u = 0; u < dist_matrix_count; u++) {
    const std::string from_location_code = dist_matrix_doc.GetCell<std::string>(
        DistMatrixSchema::headers[DistMatrixSchema::FROM_LOCATION_CODE], u);
    const std::string to_location_code = dist_matrix_doc.GetCell<std::string>(
        DistMatrixSchema::headers[DistMatrixSchema::TO_LOCATION_CODE], u);
    const std::string dist_matrix_code = dist_matrix_doc.GetCell<std::string>(
        DistMatrixSchema::headers[DistMatrixSchema::DIST_MATRIX_CODE], u);
    const long cost_dist =
        dist_matrix_doc.GetCell<long>(DistMatrixSchema::headers[DistMatrixSchema::DISTANCE], u);
    const long cost_time =
        dist_matrix_doc.GetCell<long>(DistMatrixSchema::headers[DistMatrixSchema::TIME], u);
    // find form location
    Location* from_location = location_manager->get_location(from_location_code);
    if (from_location == nullptr) {
      throw std::runtime_error("Location " + from_location_code + " was not found in " +
                               DistMatrixSchema::file_name);
    }
    Location* to_location = location_manager->get_location(to_location_code);
    if (to_location == nullptr) {
      throw std::runtime_error("Location " + to_location_code + " was not found in " +
                               DistMatrixSchema::file_name);
    }
    if (cost_dist < 0 || cost_dist > DistMatrixParameter::MAX_DISTANCE) {
      throw std::invalid_argument("cost distance " + std::to_string(cost_dist) +
                                  " was not valid in " + DistMatrixSchema::file_name);
    }
    if (cost_time < 0 || cost_time > DistMatrixParameter::MAX_TIME) {
      throw std::invalid_argument("cost time " + std::to_string(cost_time) + " was not valid in " +
                                  DistMatrixSchema::file_name);
    }
    dist_matrix_manager->set_dist_time(dist_matrix_code, from_location->ind, to_location->ind,
                                       cost_dist, cost_time);
  }
  dist_matrix_doc.Clear();
  spdlog::info("Loading {} is complete. A total of {} pieces of data have been obtained.",
               DistMatrixSchema::file_name, dist_matrix_count);

  return dist_matrix_manager;
}
