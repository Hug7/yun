/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#include "pdm_resource.h"

// ====== implement of VehicleUsage ======
VehicleUsage::UPtr VehicleUsage::deep_copy() const {
  return std::make_unique<VehicleUsage>(this->vehicle, this->max_usable, this->cur_usage);
}

void VehicleUsage::occupy_vehicle() {
  ++this->cur_usage;
  if (this->cur_usage > this->max_usable) {
    throw std::logic_error("VehicleUsage::cur_usage > max_usable");
  }
}

void VehicleUsage::release_vehicle() {
  --this->cur_usage;
  if (this->cur_usage < 0) {
    throw std::logic_error("VehicleUsage::cur_usage < 0");
  }
}

// ====== implement of VehicleUsage ======
VehicleResource::UPtr VehicleResource::deep_copy() const {
  auto copy_vehicle_usages = VehicleUsage::VecUPtr(this->len);
  for (int u = 0; u < this->len; ++u) {
    copy_vehicle_usages[u] = this->vehicle_usages[u]->deep_copy();
  }
  return std::make_unique<VehicleResource>(std::move(copy_vehicle_usages));
}

void VehicleResource::occupy(const Vehicle* vehicle) const {
  this->vehicle_usages[vehicle->ind]->occupy_vehicle();
}

void VehicleResource::release(const Vehicle* vehicle) const {
  this->vehicle_usages[vehicle->ind]->release_vehicle();
}

// ====== implement of ResourceFactory ======
VehicleResource::UPtr ResourceFactory::create_vehicle_resource(
    const CarrierManager* carrier_manager) {
  auto vehicle_usages = VehicleUsage::VecUPtr(carrier_manager->vehicle_len);
  for (int u = 0; u < carrier_manager->vehicle_len; ++u) {
    vehicle_usages[u] = std::make_unique<VehicleUsage>(carrier_manager->vehicles[u]);
  }
  return std::make_unique<VehicleResource>(std::move(vehicle_usages));
}
