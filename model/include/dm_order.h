/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <vector>

#include "c_chrono_util.h"
#include "bd_location.h"
#include "bd_cargo_order.h"

class Order;

class Order
{
public:
    /**
     * @brief order ind
     */
    int ind;
    /**
     * @brief cargo orders
     */
    std::vector<CargoOrder *> cargo_orders;
    /**
     * @brief pick location
     */
    Location *pick_loc;
    /**
     * @brief drop location
     */
    Location *drop_loc;
    /**
     * @brief pick time window
     */
    std::vector<TimeWindow *> pick_time_windows;
    /**
     * @brief drop time window
     */
    std::vector<TimeWindow *> drop_time_windows;
    /**
     * @brief 订单明细属性
     */
    std::vector<long> dim_vals;
    /**
     * @brief 订单明细标签集合值
     */
    LabelsetValue *labelset_value;
    /**
     * @brief 订单明细标签集合值位图
     */
    LabelsetValueBitset *labelset_value_bitset;

    Order(const int ind,
          std::vector<CargoOrder *> &cargo_orders,
          std::vector<long> &dim_vals,
          LabelsetValue *labelset_value,
          LabelsetValueBitset *labelset_value_bitset);

    ~Order();
};
