/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#include <format>
#include <fstream>
#include <ranges>
#include <string>

#include "c_numerical_utils.h"
#include "visual_manager.h"
#include "visual_schema.h"

// ====== implement of Load VisualManager ======
namespace {
std::vector<VisualCargoOrder> build_visual_cargo_orders(
    const std::vector<const Order*>& unassigned_orders, const Scenario* scenario) {
  const auto dim_manager = scenario->dim_manager;
  const auto order_labelset = scenario->label_manager->order_labelset;
  std::vector<VisualCargoOrder> visual_cargo_orders;
  for (const auto& order : unassigned_orders) {
    for (const auto& cargo_order : order->cargo_orders) {
      for (const auto& cargo_sub_order : cargo_order->sub_orders) {
        auto visual_cargo_order = VisualCargoOrder();
        visual_cargo_order.cargo_order_code = cargo_order->code;
        visual_cargo_order.cargo_sub_order_code = cargo_sub_order->code;
        visual_cargo_order.quantity = cargo_sub_order->qty;
        for (const auto& dim : dim_manager->dimensions) {
          visual_cargo_order.order_dimensions.emplace_back(
              dim->code,
              NumUtil::unscale_from_long(cargo_sub_order->dim_vals[dim->ind], dim->precision));
        }
        visual_cargo_order.pick_location_code = cargo_order->pick_loc->code;
        visual_cargo_order.drop_location_code = cargo_order->drop_loc->code;
        for (int u = 0; u < order_labelset->len; ++u) {
          const auto label = order_labelset->labels[u];
          std::vector<std::string> cur_label_values;

          const auto label_value_map = cargo_sub_order->labelset_value->label_values[label->ind];
          for (const auto& label_value : label_value_map | std::views::values) {
            cur_label_values.emplace_back(label_value->value);
          }
          visual_cargo_order.order_labels.emplace_back(label->code, cur_label_values);
        }

        visual_cargo_orders.emplace_back(visual_cargo_order);
      }
    }
  }

  return visual_cargo_orders;
}
}  // namespace

void VisualManager::unassigned_order_to_json(const std::vector<const Order*>& unassigned_orders,
                                             const std::string& target_dir_path) const {
  auto visual_unassigned_cargo_orders =
      build_visual_cargo_orders(unassigned_orders, this->scenario);

  const std::string file_path = target_dir_path + "/" + UNASSIGNED_CARGO_ORDERS_FILE_NAME;
  std::ofstream file(file_path);
  if (!file.is_open()) {
    this->logger->error(std::format("创建文件 {} 失败!", file_path));
    return;
  }
  file << VisualJson::dumps(visual_unassigned_cargo_orders);
  file.close();
}
