/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#include "hc_vehicle.h"

#include <cstdint>
#include <vector>

#include "pdm_resource.h"
#include "pdm_segment.h"

// ====== implement of HcVehicleCapacity ======
HardConstrScore::UPtr HcVehicleCapacity::eval(Load* load) {
  const std::vector<std::int64_t>& dim_vals = load->vehicle->vehicle_model->dim_vals;
  const std::vector<std::int64_t>& peak_load_dims = load->get_peak_load_dims();

  const size_t dim_len = dim_vals.size();
  for (size_t u = 0; u < dim_len; u++) {
    if (peak_load_dims[u] > dim_vals[u]) {
      return std::make_unique<HardConstrScore>(this->code, false, 1, 1, this->seq_type,
                                               this->vehicle_type,
                                               InfeasibleReasonCollection::VEHICLE_CAPACITY);
    }
  }
  return std::make_unique<HardConstrScore>(this->code);
}

HcPrecheckVerdict HcVehicleCapacity::precheck(const HcPrecheckInput& input) const {
  const Load* load = input.load;
  // `先提后卸` 下插入后的峰值 = 基线峰值 + segment 订单维度之和
  std::vector<std::int64_t> required = load->get_peak_load_dims();
  for (const Order* order : input.segment->orders) {
    for (size_t u = 0; u < required.size(); ++u) {
      required[u] += order->dim_vals[u];
    }
  }

  const auto fits = [&required](const Vehicle* vehicle) {
    const std::vector<std::int64_t>& capacity = vehicle->vehicle_model->dim_vals;
    for (size_t u = 0; u < required.size(); ++u) {
      if (required[u] > capacity[u]) {
        return false;
      }
    }
    return true;
  };

  // 当前车型就能装下时, 容量约束不会成为本次插入被剪枝的原因
  if (fits(load->vehicle)) {
    return HcPrecheckVerdict::kPass;
  }
  // 当前车型装不下:
  // 只要还有任一车型装得下就不能剪枝(车型占用是动态状态，预检只判定静态的容量不可行)
  if (input.vehicle_resource != nullptr) {
    for (const auto& vehicle_usage : input.vehicle_resource->vehicle_usages) {
      if (load->vehicle == vehicle_usage->vehicle) {
        continue;
      }
      if (fits(vehicle_usage->vehicle)) {
        return HcPrecheckVerdict::kPass;
      }
    }
  }
  return HcPrecheckVerdict::kInfeasible;
}
