/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string>
#include <unordered_map>

#include "bd_available_vehicle.h"
#include "c_csv_utils.h"
#include "c_file_utils.h"
#include "c_rapidcsv.h"
#include "se_schema.h"
#include "standard_csv_reader.h"

void StandardCsvReader::loading_available_vehicle(CarrierManager* carrier_manager,
                                                  LocationManager* location_manager) {
  // === loading LocationAvailableVehicle.csv ===
  spdlog::info("Loading {} ...", LocationAvailableVehicleSchema::file_name);
  const std::string location_available_vehicle_file_path =
      this->root_dir + "/" + LocationAvailableVehicleSchema::file_name;
  FileUtils::file_exists(location_available_vehicle_file_path,
                         LocationAvailableVehicleSchema::file_name);
  rapidcsv::Document location_available_vehicle_doc(location_available_vehicle_file_path);
  CsvUtils::check_column_exist(location_available_vehicle_doc,
                               LocationAvailableVehicleSchema::headers,
                               LocationAvailableVehicleSchema::file_name);

  // 站点可用车型的集合
  std::unordered_map<std::string, AvailableVehicle*> loc_available_vehicle_map;

  int location_available_vehicle_count = location_available_vehicle_doc.GetRowCount();
  for (int u = 0; u < location_available_vehicle_count; u++) {
    const std::string location_code = location_available_vehicle_doc.GetCell<std::string>(
        LocationAvailableVehicleSchema::headers[LocationAvailableVehicleSchema::LOCATION_CODE], u);
    const std::string carrier_code = location_available_vehicle_doc.GetCell<std::string>(
        LocationAvailableVehicleSchema::headers[LocationAvailableVehicleSchema::CARRIER_CODE], u);
    const std::string vehicle_model_code = location_available_vehicle_doc.GetCell<std::string>(
        LocationAvailableVehicleSchema::headers[LocationAvailableVehicleSchema::VEHICLE_MODEL_CODE],
        u);
    Location* location = location_manager->get_location(location_code);
    if (location == nullptr) {
      throw std::runtime_error("Location " + location_code + " was not found in " +
                               LocationAvailableVehicleSchema::file_name);
    }
    Carrier* carrier = carrier_manager->get_carrier(carrier_code);
    if (carrier == nullptr) {
      throw std::runtime_error("Carrier " + carrier_code + " was not found in " +
                               LocationAvailableVehicleSchema::file_name);
    }
    Vehicle* vehicle = carrier->get_vehicle(vehicle_model_code);
    if (vehicle == nullptr) {
      throw std::runtime_error("Vehicle model " + vehicle_model_code + " was not found in " +
                               LocationAvailableVehicleSchema::file_name);
    }

    if (!loc_available_vehicle_map.count(location_code)) {
      loc_available_vehicle_map[location_code] = carrier_manager->empty_available_vehicle();
    }
    loc_available_vehicle_map[location_code]->add_vehicle(vehicle);
  }
  location_available_vehicle_doc.Clear();
  spdlog::info("Loading {} is complete. A total of {} pieces of data have been obtained.",
               LocationAvailableVehicleSchema::file_name, location_available_vehicle_count);

  // 更新所有站点的可用车辆
  for (auto& [location_code, location] : location_manager->location_map) {
    auto it = loc_available_vehicle_map.find(location_code);
    if (it != loc_available_vehicle_map.end()) {
      location->set_available_vehicle(it->second);
    } else {
      location->set_available_vehicle(carrier_manager->full_available_vehicle());
    }
  }
  loc_available_vehicle_map.clear();
}
