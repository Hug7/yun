/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#include "visual_manager.h"

// ====== implement of Load VisualManager ======
void VisualManager::localization_plan_result(const std::vector<Load*>& loads,
                                             const std::vector<const Order*>& unassigned_orders,
                                             const std::string& target_dir) const {
  // 目标文件夹
  const std::string target_dir_path = this->resolve_target_dir(target_dir);
  // 将 load 转化为 visual load
  const auto visual_loads = this->loads_to_visual_loads(loads);
  // load车次维度 写入json
  this->visual_load_to_json(visual_loads, target_dir_path);
  // 将 load 转化为 visual load location
  const auto visual_load_location = this->loads_to_visual_load_locations(visual_loads, loads);
  // load站点维度 写入json
  this->visual_load_location_to_json(visual_load_location, target_dir_path);
  // 未指派订单 写入json
  this->unassigned_order_to_json(unassigned_orders, target_dir_path);
}

void VisualManager::localization_plan_result(
    const std::vector<Load*>& loads, const std::vector<const Order*>& unassigned_orders) const {
  this->localization_plan_result(loads, unassigned_orders, "");
}
