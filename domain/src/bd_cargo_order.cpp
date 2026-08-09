/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#include "bd_cargo_order.h"

// ====== implement of SubCargoOrder ======
void CargoSubOrder::update_dim_value(Dimension *dim, double dim_val)
{
    this->dim_vals[dim->ind] = static_cast<long>(dim_val * PRECISION_POW[dim->precision]);
}

void CargoSubOrder::update_labelset_value(const int label_ind4labelset, LabelValue *label_value)
{
    this->labelset_value->add_label_value(label_ind4labelset, label_value);
    this->labelset_value_bitset->add_label_value(label_ind4labelset, label_value);
}

// ====== implement of CargoOrder ======
CargoOrder::~CargoOrder()
{
    delete this->pick_time_window;

    delete this->drop_time_window;

    for (auto &sub : this->sub_orders)
    {
        delete sub;
    }
    this->sub_orders.clear();
    this->sub_order_map.clear();
}

void CargoOrder::set_pick_time_window(TimeWindow *pick_time_window)
{
    this->pick_time_window = pick_time_window;
}

void CargoOrder::set_drop_time_window(TimeWindow *drop_time_window)
{
    this->drop_time_window = drop_time_window;
}

CargoSubOrder *CargoOrder::create_cargo_sub_order(const std::string &code,
                                                  const std::string &name,
                                                  std::vector<long> &dim_vals,
                                                  LabelsetValue *labelset_value,
                                                  std::unique_ptr<LabelsetValueBitset> labelset_value_bitset,
                                                  const int qty)
{
    if (this->sub_order_map.count(code))
    {
        throw std::runtime_error("CargoSubOrder " + code + " already exists in CargoOrder " + this->code);
    }
    int ind = this->generate_index->next();
    CargoSubOrder *sub_order = new CargoSubOrder(code, name, ind, dim_vals, labelset_value, std::move(labelset_value_bitset), qty);
    this->sub_orders.push_back(sub_order);
    this->sub_order_map[code] = sub_order;

    return sub_order;
}

CargoSubOrder *CargoOrder::get_cargo_sub_order(const std::string &code)
{
    auto it = this->sub_order_map.find(code);
    if (it != this->sub_order_map.end())
    {
        return it->second;
    }

    return nullptr;
}

// ====== implement of CargoOrderManager ======
CargoOrderManager::~CargoOrderManager()
{
    for (auto order : this->cargo_orders)
    {
        delete order;
    }
    this->cargo_orders.clear();
    this->cargo_order_map.clear();
}

CargoOrder *CargoOrderManager::create_cargo_order(const std::string &code,
                                                  const std::string &name,
                                                  Location *pick_loc,
                                                  Location *drop_loc)
{
    if (this->cargo_order_map.find(code) != this->cargo_order_map.end())
    {
        throw std::runtime_error("CargoOrder " + code + " already exists in CargoOrderManager");
    }
    else
    {
        int ind = this->generate_index->next();
        CargoOrder *order = new CargoOrder(code, name, ind, pick_loc, drop_loc);
        this->cargo_orders.push_back(order);
        this->cargo_order_map[code] = order;
        this->len++;
        return order;
    }
}

CargoOrder *CargoOrderManager::get_cargo_order(const std::string &code)
{
    auto it = this->cargo_order_map.find(code);
    if (it != this->cargo_order_map.end())
    {
        return it->second;
    }

    return nullptr;
}

CargoSubOrder *CargoOrderManager::create_cargo_sub_order(
    const std::string &cargo_order_code,
    const std::string &cargo_sub_order_code,
    const std::string &cargo_sub_order_name,
    const int qty)
{
    auto it = this->cargo_order_map.find(cargo_order_code);
    if (it == this->cargo_order_map.end())
    {
        throw std::runtime_error("CargoOrder " + cargo_order_code + " does not exist in CargoOrderManager");
    }
    else
    {
        LabelsetValue *labelset_value = this->labelset->empty_labelset_value();
        auto labelset_value_bitset = this->labelset->empty_labelset_value_bitset();
        std::vector<long> dim_vals = this->dimension_manager->empty_dim_values();
        return it->second->create_cargo_sub_order(cargo_sub_order_code, cargo_sub_order_name, 
            dim_vals, labelset_value, std::move(labelset_value_bitset), qty);
    }
}
