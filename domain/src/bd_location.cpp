/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#include "bd_location.h"
#include "bd_available_vehicle.h"

// ====== implement of Location ======
Location::~Location()
{
    delete this->work_plan;
    delete this->labelset_value;
}

void Location::add_label_value(int label_ind, LabelValue *label_value)
{
    this->labelset_value->add_label_value(label_ind, label_value);
    this->labelset_value_bitset->add_label_value(label_ind, label_value);
}

void Location::set_available_vehicle(AvailableVehicle *available_vehicle)
{
    this->available_vehicle = available_vehicle;
}

// ====== implement of LocationManager ======
LocationManager::LocationManager(Labelset *labelset) : generate_index(std::make_unique<GenerateIndex>()),
                                                       locations(),
                                                       location_map(),
                                                       labelset(labelset),
                                                       len(0)
{
    // add default location
    this->create_location(LocationParameter::DEFAULT_LOCATION_CODE, LocationParameter::DEFAULT_LOCATION_CODE, 0, 0);
}

LocationManager::~LocationManager()
{
    for (auto location : this->locations)
    {
        delete location;
    }
    this->locations.clear();
    this->location_map.clear();
}

Location *LocationManager::create_location(const std::string &code, const std::string &name, const double lat, const double lng)
{
    if (this->location_map.count(code))
    {
        throw std::runtime_error("Location " + code + " already exists in LocationManager");
    }
    else
    {
        const int ind = this->generate_index->next();
        WorkPlan *work_plan = new WorkPlan();
        LabelsetValue *labelset_value = this->labelset->empty_labelset_value();
        auto labelset_value_bitset = this->labelset->empty_labelset_value_bitset();
        Location *location = new Location(code, name, ind, lat, lng, work_plan, labelset_value, std::move(labelset_value_bitset));
        this->locations.push_back(location);
        this->location_map[code] = location;
        ++this->len;
        return location;
    }
}

Location *LocationManager::get_location(const std::string &code)
{
    auto it = this->location_map.find(code);
    if (it != this->location_map.end())
    {
        return it->second;
    }
    else
    {
        return nullptr;
    }
}

Location *LocationManager::get_default_location()
{
    return this->get_location(LocationParameter::DEFAULT_LOCATION_CODE);
}
