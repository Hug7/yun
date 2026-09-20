/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#include "bdm_vehicle_model.h"

#include "c_numerical_utils.h"

// ====== implement of VehicleModel ======
VehicleModel::~VehicleModel() { delete this->labelset_value; }

void VehicleModel::update_dim_value(const Dimension* dim, const double dim_val) {
  this->dim_vals[dim->ind] = NumUtil::scale_to_long(dim_val, dim->precision);
}

void VehicleModel::update_labelset_value(const int label_ind4labelset,
                                         LabelValue* label_value) const {
  this->labelset_value->add_label_value(label_ind4labelset, label_value);
  this->labelset_value_bitset->add_label_value(label_ind4labelset, label_value);
}

// ====== implement of VehicleModelManager ======
VehicleModelManager::~VehicleModelManager() {
  for (const auto& vehicle_model : this->vehicle_models) {
    delete vehicle_model;
  }
}

VehicleModel* VehicleModelManager::create_vehicle_model(const std::string& code,
                                                        const std::string& name,
                                                        const DistMatrixCode* dist_matrix_code,
                                                        const DistMatrix* dist_matrix) {
  if (this->vehicle_model_map.contains(code)) {
    throw std::runtime_error("Vehicle Model with code " + code + " already exists.");
  }

  const int vehicle_model_ind = this->generate_index->next();
  std::vector<long> dim_vals = this->dim_manager->empty_dim_values();
  const auto vehicle_model = new VehicleModel(
      code, name, vehicle_model_ind, dist_matrix_code, dist_matrix, dim_vals,
      this->labelset->empty_labelset_value(), this->labelset->empty_labelset_value_bitset());

  this->vehicle_models.push_back(vehicle_model);
  this->vehicle_model_map[code] = vehicle_model;
  this->len++;

  return vehicle_model;
}

VehicleModel* VehicleModelManager::get_vehicle_model(const std::string& code) {
  const auto it = this->vehicle_model_map.find(code);
  if (it == this->vehicle_model_map.end()) {
    return nullptr;
  }
  return it->second;
}
