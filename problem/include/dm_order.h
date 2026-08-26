/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <memory>
#include <vector>

#include "bd_cargo_order.h"
#include "bd_common.h"
#include "bd_location.h"
#include "bd_time_window.h"
#include "dm_parameter.h"

class Order {
 public:
  /**
   * @brief order ind
   */
  const int ind;
  /**
   * @brief cargo orders
   */
  std::vector<const CargoOrder*> cargo_orders;
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

  Order(PlanDatetimeRange* plan_datetime_range, const int ind,
        std::vector<const CargoOrder*>& cargo_orders, std::vector<long>& dim_vals,
        LabelsetValue* labelset_value, LabelsetValueBitset::UPtr labelset_value_bitset,
        Bitset::UPtr available_vehicle_bitset);

  Order(PlanDatetimeRange* plan_datetime_range, std::vector<const CargoOrder*>& cargo_orders,
        std::vector<long>& dim_vals, LabelsetValue* labelset_value,
        LabelsetValueBitset::UPtr labelset_value_bitset, Bitset::UPtr available_vehicle_bitset)
      : Order(plan_datetime_range, -1, cargo_orders, dim_vals, labelset_value,
              std::move(labelset_value_bitset), std::move(available_vehicle_bitset)) {};

  ~Order();

  std::vector<TimeWindow*> copy_pick_time_windows() const;

  std::vector<TimeWindow*> copy_drop_time_windows() const;
};

class OrderManager {
 public:
  /**
   * @brief order list
   */
  std::vector<Order*> orders;
  /**
   * @brief generate index util
   */
  std::unique_ptr<GenerateIndex> generate_index;
  /**
   * @brief number of orders
   */
  int len;
  /**
   * @brief order labelset
   */
  const Labelset* labelset;
  /**
   * @brief dimension manager
   */
  const DimensionManager* dimension_manager;

  OrderManager(const Labelset* labelset, const DimensionManager* dimension_manager)
      : orders(),
        generate_index(std::make_unique<GenerateIndex>()),
        len(0),
        labelset(labelset),
        dimension_manager(dimension_manager) {};
};
