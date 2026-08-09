/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#include "standard_csv_reader.h"

void StandardCsvReader::loading_vehicle(CarrierManager *carrier_manager,
                                        VehicleModelManager *vehicle_model_manager,
                                        LocationManager *location_manager)
{
    // === loading Vehicle.csv ===
    spdlog::info("Loading {} ...", VehicleSchema::file_name);
    const std::string vehicle_file_path = this->root_dir + "/" + VehicleSchema::file_name;
    FileUtils::file_exists(vehicle_file_path, VehicleSchema::file_name);
    rapidcsv::Document vehicle_doc(vehicle_file_path);
    CsvUtils::check_column_exist(vehicle_doc, VehicleSchema::headers, VehicleSchema::file_name);

    int vehicle_count = vehicle_doc.GetRowCount();
    for (int u = 0; u < vehicle_count; u++)
    {
        const std::string carrier_code = vehicle_doc.GetCell<std::string>(VehicleSchema::headers[VehicleSchema::CARRIER_CODE], u);
        const std::string vehicle_model_code = vehicle_doc.GetCell<std::string>(VehicleSchema::headers[VehicleSchema::VEHICLE_MODEL_CODE], u);
        const int count = vehicle_doc.GetCell<int>(VehicleSchema::headers[VehicleSchema::COUNT], u);
        const std::string origin_location_code = vehicle_doc.GetCell<std::string>(VehicleSchema::headers[VehicleSchema::ORIGIN_LOCATION_CODE], u);
        const std::string destination_location_code = vehicle_doc.GetCell<std::string>(VehicleSchema::headers[VehicleSchema::DESTINATION_LOCATION_CODE], u);

        Carrier *carrier = carrier_manager->get_carrier(carrier_code);
        if (carrier == nullptr)
        {
            throw std::runtime_error("Carrier " + carrier_code + " was not found in " + VehicleSchema::file_name);
        }

        VehicleModel *vehicle_model = vehicle_model_manager->get_vehicle_model(vehicle_model_code);
        if (vehicle_model == nullptr)
        {
            throw std::runtime_error("Vehicle model " + vehicle_model_code + " was not found in " + VehicleSchema::file_name);
        }

        Location *orig_loc = location_manager->get_location(LocationParameter::DEFAULT_LOCATION_CODE);
        if (origin_location_code != "")
        {
            orig_loc = location_manager->get_location(origin_location_code);
            if (orig_loc == nullptr)
            {
                throw std::invalid_argument("Location " + origin_location_code + " was not found in " + VehicleSchema::file_name);
            }
        }

        Location *dest_loc = location_manager->get_location(LocationParameter::DEFAULT_LOCATION_CODE);
        if (destination_location_code != "")
        {
            dest_loc = location_manager->get_location(destination_location_code);
            if (dest_loc == nullptr)
            {
                throw std::invalid_argument("Location " + destination_location_code + " was not found in " + VehicleSchema::file_name);
            }
        }

        carrier_manager->create_vehicle(carrier_code, vehicle_model, count, orig_loc, dest_loc);

    }
    vehicle_doc.Clear();
    spdlog::info("Loading {} is complete. A total of {} pieces of data have been obtained.", 
        VehicleSchema::file_name, vehicle_count);

}