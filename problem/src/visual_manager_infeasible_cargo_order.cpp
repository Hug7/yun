/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#include <fstream>
#include <ranges>
#include <string>
#include <utility>
#include <vector>

#include "c_rapidcsv.h"
#include "visual_manager.h"
#include "visual_schema.h"

// ====== implement of Load VisualManager ======
/**
 * @brief 将 infeasible cargo orders 转化为 visual infeasible cargo orders
 * @param infeasible_cargo_orders 不可解订单集合
 * @return visual infeasible cargo orders
 */
std::vector<VisualInfeasibleCargoOrder> build_visual_infeasible_cargo_orders(
    const InfeasibleCargoOrder::VecUPtr& infeasible_cargo_orders) {
  std::vector<VisualInfeasibleCargoOrder> visual_infeasible_cargo_orders;
  for (const auto& infeasible_cargo_order : infeasible_cargo_orders) {
    VisualInfeasibleCargoOrder visual_infeasible_cargo_order;

    for (const auto cargo_order : infeasible_cargo_order->cargo_orders) {
      visual_infeasible_cargo_order.cargo_order_codes.push_back(cargo_order->code);
    }
    // 记录-通用的不可解原因
    for (const auto& reason :
         infeasible_cargo_order->common_infeasible_reasons | std::views::values) {
      visual_infeasible_cargo_order.common_infeasible_reasons.emplace_back(
          reason.code, reason.constr_code, reason.msg_cn, reason.msg_en);
    }

    // 记录-车辆维度不可解原因
    for (const auto& pair : infeasible_cargo_order->vehicle_infeasible_reasons) {
      VisualVehicleInfeasibleReason visual_vehicle_infeasible_reason;
      visual_vehicle_infeasible_reason.carrier_code = pair.first->carrier->code;
      visual_vehicle_infeasible_reason.vehicle_model_code = pair.first->vehicle_model->code;
      for (const auto& reason : pair.second | std::views::values) {
        visual_vehicle_infeasible_reason.infeasible_reasons.emplace_back(
            reason.code, reason.constr_code, reason.msg_cn, reason.msg_en);
      }  // end for vehicle_infeasible_reasons->val
      visual_infeasible_cargo_order.vehicle_infeasible_reasons.emplace_back(visual_vehicle_infeasible_reason);
    }  // end for vehicle_infeasible_reasons

    visual_infeasible_cargo_orders.push_back(visual_infeasible_cargo_order);
  }  // end for infeasible_cargo_orders

  return visual_infeasible_cargo_orders;
}

void VisualManager::visual_infeasible_cargo_order_to_json(
    const InfeasibleCargoOrder::VecUPtr& infeasible_cargo_orders) const {
  const auto visual_infeasible_cargo_orders =
      build_visual_infeasible_cargo_orders(infeasible_cargo_orders);
  // 写出json文件
  const std::string file_path = this->output_dir + "/" + INFEASIBLE_CARGO_ORDERS_FILE_NAME;
  std::ofstream file(file_path);
  if (!file.is_open()) {
    this->logger->error(std::format("创建文件 {} 失败!", file_path));
    return;
  }
  file << VisualJson::dumps(visual_infeasible_cargo_orders);
  file.close();
}
