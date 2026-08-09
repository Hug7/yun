/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#include "bd_carrier.h"

// ====== implement of Carrier ======
Carrier::~Carrier()
{
    vehicles.clear();
    vehicle_map.clear();
}

Vehicle *Carrier::get_vehicle(const std::string &vehicle_model_code)
{
    auto iter = this->vehicle_map.find(vehicle_model_code);
    if (iter == this->vehicle_map.end())
    {
        return nullptr;
    }
    return iter->second;
}

void Carrier::update_labelset_value(const int label_ind4labelset, LabelValue *label_value)
{
    this->labelset_value->add_label_value(label_ind4labelset, label_value);
    this->labelset_value_bitset->add_label_value(label_ind4labelset, label_value);
}

// ====== implement of Carrier ======
CarrierManager::~CarrierManager()
{
    for (auto &carrier : carriers)
    {
        delete carrier;
    }
    carriers.clear();
    carrier_map.clear();
    for (auto &vehicle : vehicles)
    {
        delete vehicle;
    }
    vehicles.clear();
}

Carrier *CarrierManager::create_carrier(const std::string &code, const std::string &name)
{
    if (this->carrier_map.count(code))
    {
        throw new std::runtime_error("Carrier with code " + code + " already exists.");
    }
    Carrier *carrier = new Carrier(code, name, this->carrier_generate_index->next(),
                                   this->labelset->empty_labelset_value(), this->labelset->empty_labelset_value_bitset());

    this->carriers.push_back(carrier);
    this->carrier_map[code] = carrier;

    return carrier;
}

Carrier *CarrierManager::get_carrier(const std::string &code)
{
    auto iter = this->carrier_map.find(code);
    if (iter == this->carrier_map.end())
    {
        return nullptr;
    }
    return iter->second;
}

Vehicle *CarrierManager::create_vehicle(const std::string &carrier_code,
                                        const VehicleModel *vehicle_model,
                                        const int count,
                                        const Location *orig_loc,
                                        const Location *dest_loc)
{
    Carrier *carrier = this->get_carrier(carrier_code);
    if (carrier == nullptr)
    {
        throw new std::runtime_error("Carrier with code " + carrier_code + " does not exist.");
    }
    Vehicle *vehicle = new Vehicle(carrier, this->vehicle_generate_index->next(), vehicle_model, count, orig_loc, dest_loc);
    this->vehicles.push_back(vehicle);
    carrier->vehicles.push_back(vehicle);
    carrier->vehicle_map[vehicle_model->code] = vehicle;
    return vehicle;
}

std::unique_ptr<Bitset> CarrierManager::empty_vehicle_bitset()
{
    return std::make_unique<Bitset>(this->vehicles.size());
}

std::unique_ptr<Bitset> CarrierManager::full_vehicle_bitset()
{
    auto bitset = std::make_unique<Bitset>(this->vehicles.size());
    bitset->set_all();
    return bitset;
}

AvailableVehicle *CarrierManager::empty_available_vehicle()
{
    std::vector<Vehicle *> cur_vehicles = std::vector<Vehicle *>();
    return new AvailableVehicle(cur_vehicles, this->empty_vehicle_bitset());
}

AvailableVehicle *CarrierManager::full_available_vehicle()
{
    std::vector<Vehicle *> cur_vehicles = this->vehicles;
    return new AvailableVehicle(cur_vehicles, this->full_vehicle_bitset());
}