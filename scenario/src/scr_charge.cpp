/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#include "scr_charge_reader.h"
#include "scr_standard_csv_reader.h"

ChargeManager* StandardCsvReader::loading_charge(DimensionManager* dimension_manager,
                                                 LabelManager* label_manager,
                                                 CarrierManager* carrier_manager,
                                                 VehicleModelManager* vehicle_model_manager,
                                                 LocationManager* location_manager) const {
  return ChargeCsvReader(this->root_dir, this->logger, dimension_manager, label_manager,
                         carrier_manager, vehicle_model_manager, location_manager)
      .load();
}
