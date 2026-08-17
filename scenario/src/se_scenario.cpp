/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#include "se_scenario.h"

Scenario::~Scenario() {
  delete this->parameter;
  delete this->dimension_manager;
  delete this->label_manager;
  delete this->vehicle_model_manager;
  delete this->carrier_manager;
  delete this->location_manager;
  delete this->dist_matrix_manager;
  delete this->cargo_order_manager;
}

void Scenario::set_parameter(Parameter* parameter) { this->parameter = parameter; }

void Scenario::set_dimension_manager(DimensionManager* dimension_manager) {
  this->dimension_manager = dimension_manager;
}

void Scenario::set_label_manager(LabelManager* label_manager) {
  this->label_manager = label_manager;
}

void Scenario::set_vehicle_model_manager(VehicleModelManager* vehicle_model_manager) {
  this->vehicle_model_manager = vehicle_model_manager;
}

void Scenario::set_carrier_manager(CarrierManager* carrier_manager) {
  this->carrier_manager = carrier_manager;
}

void Scenario::set_location_manager(LocationManager* location_manager) {
  this->location_manager = location_manager;
}

void Scenario::set_dist_matrix_manager(DistMatrixManager* dist_matrix_manager) {
  this->dist_matrix_manager = dist_matrix_manager;
}

void Scenario::set_cargo_order_manager(CargoOrderManager* cargo_order_manager) {
  this->cargo_order_manager = cargo_order_manager;
}
