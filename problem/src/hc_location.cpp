/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#include "hc_location.h"

#include "pdm_load.h"

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

// ====== implement of HcTimeWindow ======
HardConstrScore::UPtr HcTimeWindow::eval(Load* load) {
  // 更新并时间窗
  load->update_time_window();

  auto tail_node = load->last_node;
  while (tail_node != nullptr) {
    if (tail_node->ptws.empty() || tail_node->ptws[0]->over_time > 0) {
      return std::make_unique<HardConstrScore>(this->code, false, 1, 1, this->seq_type,
                                               this->vehicle_type,
                                               InfeasibleReasonCollection::TIME_WINDOW);
    }
    tail_node = tail_node->prev;
  }

  return std::make_unique<HardConstrScore>(this->code);
}
