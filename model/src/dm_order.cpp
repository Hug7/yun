/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#include "bd_time_window_utils.h"
#include "dm_order.h"

Order::Order(const int ind,
             std::vector<CargoOrder *> &cargo_orders,
             std::vector<long> &dim_vals,
             LabelsetValue *labelset_value,
             LabelsetValueBitset *labelset_value_bitset)
{
    this->ind = ind;
    this->cargo_orders = cargo_orders;
    this->pick_loc = cargo_orders[0]->pick_loc;
    this->drop_loc = cargo_orders[0]->drop_loc;
    // intersection pick and drop time windows of all cargo orders
    this->pick_time_windows.push_back(new TimeWindow());
    this->drop_time_windows.push_back(new TimeWindow());
    for (const auto &cargo_order : cargo_orders)
    {
        // process pick time window
        std::vector<TimeWindow *> tmp_pick_time_windows;
        TimeWindow *cur_pick_tw = cargo_order->pick_time_window;
        for (auto &ptw : this->pick_time_windows)
        {
            TimeWindow *tw = TimeWindowUntils::intersection(cur_pick_tw, ptw);
            if (tw != nullptr)
            {
                tmp_pick_time_windows.push_back(tw);
            }
        }
        for (auto &tw : this->pick_time_windows)
        {
            delete tw;
        }
        this->pick_time_windows = tmp_pick_time_windows;
        // process drop time window
        std::vector<TimeWindow *> tmp_drop_time_windows;
        TimeWindow *cur_drop_tw = cargo_order->drop_time_window;
        for (auto &dtw : this->drop_time_windows)
        {
            TimeWindow *tw = TimeWindowUntils::intersection(cur_drop_tw, dtw);
            if (tw != nullptr)
            {
                tmp_drop_time_windows.push_back(tw);
            }
        }
        for (auto &tw : this->drop_time_windows)
        {
            delete tw;
        }
        this->drop_time_windows = tmp_drop_time_windows;
    }
    // intersection pick and drop time windows for location calendar
    Calendar *pick_calendar = this->pick_loc->work_plan->pick_calendar;
    Calendar *drop_calendar = this->pick_loc->work_plan->drop_calendar;
    std::vector<TimeWindow *> res_pick_time_windows;
    for (auto &tw : this->pick_time_windows)
    {
        std::vector<TimeWindow *> extend_tws = pick_calendar->intersection(tw);
        res_pick_time_windows.insert(res_pick_time_windows.end(), extend_tws.begin(), extend_tws.end());
        delete tw;
    }
    this->pick_time_windows = TimeWindowUntils::merge_time_windows(res_pick_time_windows);

    std::vector<TimeWindow *> res_drop_time_windows;
    for (auto &tw : this->drop_time_windows)
    {
        std::vector<TimeWindow *> extend_tws = drop_calendar->intersection(tw);
        res_drop_time_windows.insert(res_drop_time_windows.end(), extend_tws.begin(), extend_tws.end());
        delete tw;
    }
    this->drop_time_windows = TimeWindowUntils::merge_time_windows(res_drop_time_windows);

    // accumulate sub cargo order
    // 1. accumulate the dim values
    // 2. accumulate labelset value
    this->dim_vals = dim_vals;
    this->labelset_value = labelset_value;
    this->labelset_value_bitset = labelset_value_bitset;
    const int dim_vals_len = this->dim_vals.size();
    for (const auto &cargo_order : cargo_orders)
    {
        for (const auto &sub_order : cargo_order->sub_orders)
        {
            // accumulate the dim values
            for (int u = 0; u < dim_vals_len; u++)
            {
                this->dim_vals[u] += sub_order->dim_vals[u];
            }
            // accumulate labelset value
            this->labelset_value->merge(sub_order->labelset_value);
            this->labelset_value_bitset->merge(sub_order->labelset_value_bitset);
        }
    }
}

Order::~Order()
{
    this->cargo_orders.clear();
    for (auto &tw : this->pick_time_windows)
    {
        delete tw;
    }
    this->pick_time_windows.clear();
    for (auto &tw : this->drop_time_windows)
    {
        delete tw;
    }
    this->drop_time_windows.clear();
    delete this->labelset_value;
    delete this->labelset_value_bitset;
}