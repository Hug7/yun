/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <spdlog/spdlog.h>

#include <memory>
#include <string>

#include "pdm_infeasible_order.h"

class VisualManager {
 private:
  /**
   * @brief 输出文件目录
   */
  const std::string output_dir;
  /**
   * @brief 日志器，本次请求专属
   */
  const std::shared_ptr<spdlog::logger> logger;

 public:
  VisualManager(const std::string& output_dir, const std::shared_ptr<spdlog::logger>& logger);

  /**
   * @brief 不可解订单生成csv文件
   * @param infeasible_cargo_orders 不可解订单集合
   */
  void infeasible_cargo_order_to_csv(InfeasibleCargoOrder::VecUPtr& infeasible_cargo_orders);
};
