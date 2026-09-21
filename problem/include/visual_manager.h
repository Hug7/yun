/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <spdlog/spdlog.h>

#include <memory>
#include <string>
#include <vector>

#include "pdm_infeasible_order.h"
#include "pdm_load.h"
#include "pdm_order.h"
#include "visual_plan_result_load.h"

/**
 * @brief 可视化管理器
 */
class VisualManager {
 private:
  /**
   * @brief 输出文件目录
   */
  const std::string output_dir;
  /**
   * @brief 场景数据
   */
  const Scenario* scenario;
  /**
   * @brief 日志器，本次请求专属
   */
  const std::shared_ptr<spdlog::logger> logger;
  /**
   * @brief 解析目标目录, 不存在则创建
   * @param target_dir 目标目录名, 为空时直接使用输出目录
   * @return 目标目录的完整路径
   */
  [[nodiscard]] std::string resolve_target_dir(const std::string& target_dir) const;
  /**
   * @brief 将 load 转化为 visual load
   * @param loads 车次列表
   * @return visual loads
   */
  [[nodiscard]] std::vector<VisualLoad> loads_to_visual_loads(
      const std::vector<Load*>& loads) const;
  /**
   * @brief 将 load 转化为 visual load
   * @param visual_loads 可视化车次列表
   * @param loads 车次列表
   * @return visual loads
   */
  [[nodiscard]] std::vector<VisualLoadLocation> loads_to_visual_load_locations(
      const std::vector<VisualLoad>& visual_loads, const std::vector<Load*>& loads) const;

 public:
  VisualManager(std::string output_dir, const Scenario* scenario,
                const std::shared_ptr<spdlog::logger>& logger);

  /**
   * @brief 不可解订单生成csv文件
   * @param infeasible_cargo_orders 不可解订单集合
   */
  void infeasible_cargo_order_to_csv(
      const InfeasibleCargoOrder::VecUPtr& infeasible_cargo_orders) const;
  /**
   * @brief 不可解订单生成json文件
   * @param infeasible_cargo_orders 不可解订单集合
   */
  void infeasible_cargo_order_to_json(
      const InfeasibleCargoOrder::VecUPtr& infeasible_cargo_orders) const;
  /**
   * @brief 将 visual loads 转换为车次维度的json文件
   * @param visual_loads 结果车次列表
   * @param target_dir_path 目标目录路径
   */
  void visual_load_to_json(const std::vector<VisualLoad>& visual_loads,
                           const std::string& target_dir_path) const;
  /**
   * @brief 将可视化车次-站点维度转换为json
   * @param visual_load_locations 站点维度可视化车次列表
   * @param target_dir_path 目标目录路径
   */
  void visual_load_location_to_json(const std::vector<VisualLoadLocation>& visual_load_locations,
                                    const std::string& target_dir_path) const;
  /**
   * @brief 将未指派订单转换为json
   * @param unassigned_orders 未指派订单
   * @param target_dir_path 目标目录路径
   */
  void unassigned_order_to_json(const std::vector<const Order*>& unassigned_orders,
                                const std::string& target_dir_path) const;

  /**
   * @brief 将规划结果本地化
   * @param loads 车次列表
   * @param unassigned_orders 未指派订单
   * @param target_dir 目标目录
   */
  void localization_plan_result(const std::vector<Load*>& loads,
                                const std::vector<const Order*>& unassigned_orders,
                                const std::string& target_dir) const;

  /**
   * @brief 将规划结果本地化
   * @param loads 车次列表
   * @param unassigned_orders 未指派订单
   */
  void localization_plan_result(const std::vector<Load*>& loads,
                                const std::vector<const Order*>& unassigned_orders) const;
};
