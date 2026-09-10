/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <memory>
#include <vector>

#include "bdm_cargo_order.h"
#include "bdm_common.h"
#include "bdm_location.h"
#include "bdm_time_window.h"
#include "pdm_parameter.h"
#include "se_scenario.h"

/**
 * @brief 订单-problem层
 */
class Order {
 public:
  /**
   * @brief order ind
   */
  const int ind;
  /**
   * @brief cargo orders
   */
  std::vector<CargoOrder*> cargo_orders;
  /**
   * @brief pick location
   */
  const Location* pick_loc;
  /**
   * @brief drop location
   */
  const Location* drop_loc;
  /**
   * @brief pick time windows
   */
  std::vector<TimeWindow*> pick_time_windows;
  /**
   * @brief length of pick time windows
   */
  int pick_time_windows_len;
  /**
   * @brief drop time windows
   */
  std::vector<TimeWindow*> drop_time_windows;
  /**
   * @brief length of drop time windows
   */
  int drop_time_windows_len;
  /**
   * @brief pick work time
   */
  long pick_work_time;
  /**
   * @brief drop work time
   */
  long drop_work_time;
  /**
   * @brief 订单明细属性
   */
  std::vector<long> dim_vals;
  /**
   * @brief 订单明细标签集合值
   */
  LabelsetValue* labelset_value;
  /**
   * @brief 订单明细标签集合值位图
   */
  LabelsetValueBitset::UPtr labelset_value_bitset;
  /**
   * @brief available vehicle bitset
   */
  Bitset::UPtr available_vehicle_bitset;

  Order(const PlanDatetimeRange* plan_datetime_range, int ind,
        std::vector<CargoOrder*>& cargo_orders, std::vector<long>& dim_vals,
        LabelsetValue* labelset_value, LabelsetValueBitset::UPtr labelset_value_bitset,
        Bitset::UPtr available_vehicle_bitset);

  ~Order();

  [[nodiscard]] std::vector<TimeWindow*> copy_pick_time_windows() const;

  [[nodiscard]] std::vector<TimeWindow*> copy_drop_time_windows() const;
};

namespace OrderFactory {
/**
 * @brief 创建order
 * @param cargo_orders cargo order列表
 * @param ind order的索引
 * @param plan_datetime_range 规划时间范围
 * @param scenario 场景
 * @return order对象指针
 */
Order* creat_order(std::vector<CargoOrder*>& cargo_orders, int ind,
                   const PlanDatetimeRange* plan_datetime_range, const Scenario* scenario);
/**
 * @brief 创建临时order
 * @param cargo_orders cargo order列表
 * @param plan_datetime_range 规划时间范围
 * @param scenario 场景
 * @return 临时order对象指针
 */
Order* creat_tmp_order(std::vector<CargoOrder*>& cargo_orders,
                       const PlanDatetimeRange* plan_datetime_range, const Scenario* scenario);
}  // namespace OrderFactory
