/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#include <format>

#include "bd_carrier.h"
#include "bd_available_vehicle.h"

// ====== implement of AvailableVehicle ======
AvailableVehicle::~AvailableVehicle()
{
    this->vehicles.clear();
}

void AvailableVehicle::add_vehicle(Vehicle *vehicle)
{
    if (this->vehicle_bitset->test(vehicle->ind))
    {
        throw std::invalid_argument(std::format("a vehicle with the carrier {} and the vehicle model {} is duplicated in available vehicle",
        vehicle->carrier->code, vehicle->vehicle_model->code));
    }
    this->vehicles.push_back(vehicle);
    // sorting vehicles by vehilce index
    std::sort(this->vehicles.begin(), this->vehicles.end(), CompareByVehicleInd());
    this->vehicle_bitset->set(vehicle->ind);
}
