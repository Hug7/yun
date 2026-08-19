/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#include "hc_load_unload_policy.h"

#include <algorithm>
#include <vector>

namespace {

struct LoadUnloadPosition {
  const Order* order;
  int pick_node_index;
  int drop_node_index;
};

HardConstrScore::UPtr eval_load_unload_policy(Load* load, bool filo, const std::string& code,
                                              bool seq_type, bool vehicle_type,
                                              const InfeasibleReason& reason) {
  std::vector<LoadUnloadPosition> positions;
  int node_index = 0;
  for (Node* node = load->first_node->next.get(); node != nullptr;
       node = node->next.get(), ++node_index) {
    for (Activity* activity = node->first.get(); activity != nullptr;
         activity = activity->next.get()) {
      if (activity->order == nullptr) {
        continue;
      }

      if (activity->activity_type == ActivityType::PICK) {
        positions.push_back({activity->order, node_index, -1});
      } else if (activity->activity_type == ActivityType::DROP) {
        auto it = std::find_if(
            positions.begin(), positions.end(), [&](const LoadUnloadPosition& position) {
              return position.order == activity->order && position.drop_node_index < 0;
            });
        if (it != positions.end()) {
          it->drop_node_index = node_index;
        }
      }
    }
  }

  positions.erase(std::remove_if(positions.begin(), positions.end(),
                                 [](const LoadUnloadPosition& position) {
                                   return position.pick_node_index < 0 ||
                                          position.drop_node_index < 0;
                                 }),
                  positions.end());

  if (positions.empty()) {
    return std::make_unique<HardConstrScore>(code);
  }

  // Activities in the same node are simultaneous, so keep their drop order
  // aligned with the requested policy to avoid false violations.
  std::sort(positions.begin(), positions.end(),
            [filo](const LoadUnloadPosition& lhs, const LoadUnloadPosition& rhs) {
              if (lhs.pick_node_index != rhs.pick_node_index) {
                return lhs.pick_node_index < rhs.pick_node_index;
              }
              return filo ? lhs.drop_node_index > rhs.drop_node_index
                          : lhs.drop_node_index < rhs.drop_node_index;
            });

  int last_keep_index = positions.front().drop_node_index;
  for (auto it = positions.begin() + 1; it != positions.end(); ++it) {
    if (filo ? it->drop_node_index > last_keep_index : it->drop_node_index < last_keep_index) {
      return std::make_unique<HardConstrScore>(code, false, 1, 1, seq_type, vehicle_type, reason);
    }
    last_keep_index = filo ? std::min(last_keep_index, it->drop_node_index)
                           : std::max(last_keep_index, it->drop_node_index);
  }

  return std::make_unique<HardConstrScore>(code);
}

}  // namespace

// ====== implement of HcLoadUnloadPolicyFILO ======
HardConstrScore::UPtr HcLoadUnloadPolicyFILO::eval(Load* load) {
  return eval_load_unload_policy(load, true, this->code, this->seq_type, this->vehicle_type,
                                 InfeasibleReasonCollection::FILO_LOAD_UNLOAD_POLICY);
}

// ====== implement of HcLoadUnloadPolicyFIFO ======
HardConstrScore::UPtr HcLoadUnloadPolicyFIFO::eval(Load* load) {
  return eval_load_unload_policy(load, false, this->code, this->seq_type, this->vehicle_type,
                                 InfeasibleReasonCollection::FIFO_LOAD_UNLOAD_POLICY);
}
