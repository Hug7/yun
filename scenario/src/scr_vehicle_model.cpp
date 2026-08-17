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

VehicleModelManager* StandardCsvReader::loading_vehicle_model(
    DimensionManager* dimension_manager, LabelManager* label_manager,
    DistMatrixManager* dist_matrix_manager) {
  // === loading VehicleModel.csv ===
  spdlog::info("Loading {} ...", VehicleModelSchema::file_name);
  const std::string vehicle_model_file_path = this->root_dir + "/" + VehicleModelSchema::file_name;
  FileUtils::file_exists(vehicle_model_file_path, VehicleModelSchema::file_name);
  rapidcsv::Document vehicle_model_doc(vehicle_model_file_path);
  CsvUtils::check_column_exist(vehicle_model_doc, VehicleModelSchema::headers,
                               VehicleModelSchema::file_name);

  VehicleModelManager* vehicle_model_manager =
      new VehicleModelManager(dimension_manager, label_manager->vehicle_model_labelset);

  int vehicle_model_count = vehicle_model_doc.GetRowCount();
  for (int u = 0; u < vehicle_model_count; u++) {
    const std::string vehicle_model_code = vehicle_model_doc.GetCell<std::string>(
        VehicleModelSchema::headers[VehicleModelSchema::CODE], u);
    const std::string vehicle_model_name = vehicle_model_doc.GetCell<std::string>(
        VehicleModelSchema::headers[VehicleModelSchema::NAME], u);
    const std::string dist_matrix_code_str = vehicle_model_doc.GetCell<std::string>(
        VehicleModelSchema::headers[VehicleModelSchema::DIST_MATRIX_CODE], u);

    DistMatrixCode* dist_matrix_code =
        dist_matrix_manager->get_dist_matrix_code(dist_matrix_code_str);
    DistMatrix* dist_matrix = dist_matrix_manager->get_dist_matrix(dist_matrix_code);

    vehicle_model_manager->create_vehicle_model(vehicle_model_code, vehicle_model_name,
                                                dist_matrix_code, dist_matrix);
  }
  vehicle_model_doc.Clear();
  spdlog::info("Loading {} is complete. A total of {} pieces of data have been obtained.",
               VehicleModelSchema::file_name, vehicle_model_count);

  // === loading VehicleModelDimensionValue.csv ===
  spdlog::info("Loading {} ...", VehicleModelDimensionValueSchema::file_name);
  const std::string vehicle_model_dim_val_file_path =
      this->root_dir + "/" + VehicleModelDimensionValueSchema::file_name;
  FileUtils::file_exists(vehicle_model_dim_val_file_path,
                         VehicleModelDimensionValueSchema::file_name);
  rapidcsv::Document vehicle_model_dim_val_doc(vehicle_model_dim_val_file_path);
  CsvUtils::check_column_exist(vehicle_model_dim_val_doc, VehicleModelDimensionValueSchema::headers,
                               VehicleModelDimensionValueSchema::file_name);

  int vehicle_model_dim_val_count = vehicle_model_dim_val_doc.GetRowCount();

  for (int u = 0; u < vehicle_model_dim_val_count; u++) {
    const std::string vehicle_model_code = vehicle_model_dim_val_doc.GetCell<std::string>(
        VehicleModelDimensionValueSchema::headers
            [VehicleModelDimensionValueSchema::VEHICLE_MODEL_CODE],
        u);
    const std::string dimension_code = vehicle_model_dim_val_doc.GetCell<std::string>(
        VehicleModelDimensionValueSchema::headers[VehicleModelDimensionValueSchema::DIMENSION_CODE],
        u);
    const double dimension_value = vehicle_model_dim_val_doc.GetCell<double>(
        VehicleModelDimensionValueSchema::headers
            [VehicleModelDimensionValueSchema::DIMENSION_VALUE],
        u);

    VehicleModel* vehicle_model = vehicle_model_manager->get_vehicle_model(vehicle_model_code);
    if (vehicle_model == nullptr) {
      throw std::runtime_error("Vehicle model not found: " + vehicle_model_code + " in " +
                               VehicleModelDimensionValueSchema::file_name);
    }

    Dimension* dimension = dimension_manager->get_dimension(dimension_code);
    if (dimension == nullptr) {
      throw std::runtime_error("Dimension not found: " + dimension_code + " in " +
                               VehicleModelDimensionValueSchema::file_name);
    }

    vehicle_model->update_dim_value(dimension, dimension_value);
  }
  vehicle_model_dim_val_doc.Clear();
  spdlog::info("Loading {} is complete. A total of {} pieces of data have been obtained.",
               VehicleModelDimensionValueSchema::file_name, vehicle_model_dim_val_count);

  // === loading VehicleModelDimensionValue.csv ===
  spdlog::info("Loading {} ...", VehicleModelLabelValueSchema::file_name);
  const std::string vehicle_model_label_value_file_path =
      root_dir + "/" + VehicleModelLabelValueSchema::file_name;
  FileUtils::file_exists(vehicle_model_label_value_file_path,
                         VehicleModelLabelValueSchema::file_name);
  rapidcsv::Document vehicle_model_label_value_doc(vehicle_model_label_value_file_path);
  CsvUtils::check_column_exist(vehicle_model_label_value_doc, VehicleModelLabelValueSchema::headers,
                               VehicleModelLabelValueSchema::file_name);

  int vehicle_model_label_value_count = vehicle_model_label_value_doc.GetRowCount();

  for (int u = 0; u < vehicle_model_label_value_count; u++) {
    const std::string vehicle_model_code = vehicle_model_label_value_doc.GetCell<std::string>(
        VehicleModelLabelValueSchema::headers[VehicleModelLabelValueSchema::VEHICLE_MODEL_CODE], u);
    const std::string label_code = vehicle_model_label_value_doc.GetCell<std::string>(
        VehicleModelLabelValueSchema::headers[VehicleModelLabelValueSchema::LABEL_CODE], u);
    const std::string label_value_code = vehicle_model_label_value_doc.GetCell<std::string>(
        VehicleModelLabelValueSchema::headers[VehicleModelLabelValueSchema::LABEL_VALUE], u);
    // check vehicle model
    VehicleModel* vehicle_model = vehicle_model_manager->get_vehicle_model(vehicle_model_code);
    if (vehicle_model == nullptr) {
      throw std::runtime_error("Vehicle model " + vehicle_model_code + " was not found in " +
                               VehicleModelLabelValueSchema::file_name);
    }
    // check label
    Label* label = label_manager->get_label(label_code);
    if (label == nullptr) {
      throw std::runtime_error("Label " + label_code + " was not found in " +
                               VehicleModelLabelValueSchema::file_name);
    }
    // check label value in label
    LabelValue* label_value = label->get_label_value(label_value_code);
    if (label_value == nullptr) {
      throw std::runtime_error("Label value " + label_value_code + " was not found for Label " +
                               label_code + " in " + VehicleModelLabelValueSchema::file_name);
    }
    // check label in vehicle model
    int label_ind4labelset = vehicle_model_manager->labelset->get_label_ind(label_code);
    if (label_ind4labelset == -1) {
      throw std::runtime_error("Label " + label_code +
                               " was not found in labelset of vehilce model" + vehicle_model_code);
    }
    // update labelset value
    vehicle_model->update_labelset_value(label_ind4labelset, label_value);
  }
  vehicle_model_label_value_doc.Clear();
  spdlog::info("Loading {} is complete. A total of {} pieces of data have been obtained.",
               VehicleModelLabelValueSchema::file_name, vehicle_model_label_value_count);

  return vehicle_model_manager;
}
