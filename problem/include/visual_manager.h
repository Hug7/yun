/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <string>

#include "pdm_infeasible_order.h"

class VisualManager {
 private:
  const std::string file_path;

 public:
  explicit VisualManager(const std::string& root_dir, const std::string& request_id);

  /**
   * @brief 不可解订单生成csv文件
   * @param infeasible_cargo_orders 不可解订单集合
   */
  void infeasible_cargo_order_to_csv(InfeasibleCargoOrder::VecUPtr& infeasible_cargo_orders);
};