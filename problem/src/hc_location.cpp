/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#include "hc_location.h"

#include "dm_load.h"

// ====== implement of HcMaxPickNodeCount ======
HardConstrScore::UPtr HcMaxPickNodeCount::eval(Load* load) {
  if (load->get_pick_node_count() > this->max_pick_node_count) {
    return std::make_unique<HardConstrScore>(this->code, false, 1, 1, this->seq_type,
                                             this->vehicle_type,
                                             InfeasibleReasonCollection::MAX_PICK_NODE_COUNT);
  }
  return std::make_unique<HardConstrScore>(this->code);
}

// ====== implement of HcMaxDropNodeCount ======
HardConstrScore::UPtr HcMaxDropNodeCount::eval(Load* load) {
  if (load->get_drop_node_count() > this->max_drop_node_count) {
    return std::make_unique<HardConstrScore>(this->code, false, 1, 1, this->seq_type,
                                             this->vehicle_type,
                                             InfeasibleReasonCollection::MAX_DROP_NODE_COUNT);
  }
  return std::make_unique<HardConstrScore>(this->code);
}

// ====== implement of HcAvailableVehicle ======
HardConstrScore::UPtr HcAvailableVehicle::eval(Load* load) {

  auto available_vehicle_bitset = load->get_available_vehicle_bitset();

  if (!available_vehicle_bitset->test(load->vehicle->ind)) {
    return std::make_unique<HardConstrScore>(this->code, false, 1, 1, this->seq_type,
                                             this->vehicle_type,
                                             InfeasibleReasonCollection::AVAILABLE_VEHICLE);
  }

  return std::make_unique<HardConstrScore>(this->code);
}
