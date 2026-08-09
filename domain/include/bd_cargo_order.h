/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <ctime>
#include <memory>
#include <climits>

#include "bd_common.h"
#include "bd_label.h"
#include "bd_dimension.h"
#include "bd_location.h"
#include "bd_time_window.h"

/**
 * @brief 子订单明细
 */
class CargoSubOrder : public Item
{
public:
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
    std::unique_ptr<LabelsetValueBitset> labelset_value_bitset;
    /**
     * @brief 货量
     */
    const int qty;

    CargoSubOrder(const std::string &code,
                  const std::string &name,
                  const int ind,
                  std::vector<long> &dim_vals,
                  LabelsetValue *labelset_value,
                  std::unique_ptr<LabelsetValueBitset> labelset_value_bitset,
                  const int qty) : Item(code, name, ind),
                                   dim_vals(dim_vals),
                                   labelset_value(labelset_value),
                                   labelset_value_bitset(std::move(labelset_value_bitset)),
                                   qty(qty) {}

    /**
     * @brief update value of specified dimension
     * @param dim dimension
     * @param dim_val value of dimension
     */
    void update_dim_value(Dimension *dim, double dim_val);

    /**
     * @brief Update labelset value and labelset value bitset
     * @param label_ind4labelset index of label in labelset
     * @param label_value label value
     */
    void update_labelset_value(const int label_ind4labelset, LabelValue *label_value);
};

/**
 * @brief 订单
 */
class CargoOrder : public Item
{
private:
    /**
     * @brief 订单明细索引生成器
     */
    std::unique_ptr<GenerateIndex> generate_index;

public:
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
    TimeWindow *pick_time_window;
    /**
     * @brief drop time window
     */
    TimeWindow *drop_time_window;
    /**
     * @brief 订单明细
     */
    std::vector<CargoSubOrder *> sub_orders;
    /**
     * @brief 订单明细
     */
    std::unordered_map<std::string, CargoSubOrder *> sub_order_map;

    CargoOrder(const std::string &code, const std::string &name, const int ind,
               Location *pick_loc,
               Location *drop_loc) : Item(code, name, ind),
                                     generate_index(std::make_unique<GenerateIndex>()),
                                     pick_loc(pick_loc),
                                     drop_loc(drop_loc),
                                     pick_time_window(nullptr),
                                     drop_time_window(nullptr),
                                     sub_orders(),
                                     sub_order_map() {}

    ~CargoOrder();
    /**
     * @brief 设置提货时间窗
     */
    void set_pick_time_window(TimeWindow *pick_time_window);
    /**
     * @brief 设置卸货时间窗
     */
    void set_drop_time_window(TimeWindow *drop_time_window);
    /**
     * @brief 创建子订单
     * @param code 子订单编码
     * @param name 子订单名称
     * @param dim_vals 子订单各维度数据
     * @param labelset_value 子订单标签集合值
     * @param labelset_value_bitset 子订单标签集合值位图
     * @param qty 订单明细数量
     * @return 订单明细对象指针
     */
    CargoSubOrder *create_cargo_sub_order(const std::string &code, const std::string &name, std::vector<long> &dim_vals, 
        LabelsetValue *labelset_value, std::unique_ptr<LabelsetValueBitset> labelset_value_bitset, const int qty);
    /**
     * @brief 获取子订单
     * @param code 子订单编码
     */
    CargoSubOrder *get_cargo_sub_order(const std::string &code);
};

/**
 * @brief 订单管理类
 * @details 订单管理类用于管理所有订单
 */
class CargoOrderManager
{
public:
    /**
     * @brief 订单列表
     */
    std::vector<CargoOrder *> cargo_orders;
    /**
     * @brief 订单映射表
     */
    std::unordered_map<std::string, CargoOrder *> cargo_order_map;
    /**
     * @brief 订单索引生成器
     */
    std::unique_ptr<GenerateIndex> generate_index;
    /**
     * @brief 订单数量
     */
    int len;
    /**
     * @brief 订单标签集合
     */
    Labelset *labelset;
    /**
     * @brief 维度管理器
     * @details 维度管理器用于管理所有订单的维度
     */
    DimensionManager *dimension_manager;

    CargoOrderManager(
        Labelset *labelset,
        DimensionManager *dimension_manager) : cargo_orders(),
                                               cargo_order_map(),
                                               generate_index(std::make_unique<GenerateIndex>()),
                                               len(0),
                                               labelset(labelset),
                                               dimension_manager(dimension_manager) {}

    ~CargoOrderManager();
    /**
     * @brief 创建订单
     * @param code 订单编码
     * @param name 订单名称
     * @param pick_loc 提货站点对象指针
     * @param drop_loc 卸货站点对象指针
     * @return 订单对象指针，若已存在则抛出异常
     */
    CargoOrder *create_cargo_order(const std::string &code, const std::string &name, Location *pick_loc, Location *drop_loc);

    /**
     * @brief 获取订单
     * @param code 订单编码
     * @return 订单对象指针
     */
    CargoOrder *get_cargo_order(const std::string &code);
    /**
     * @brief 创建子订单
     * @param cargo_order_code 订单编码
     * @param cargo_sub_order_code 子订单编码
     * @param cargo_sub_order_name 子订单名称
     * @param qty 订单明细数量
     * @return 子订单对象指针，若订单不存在则抛出异常
     */
    CargoSubOrder *create_cargo_sub_order(
        const std::string &cargo_order_code,
        const std::string &cargo_sub_order_code,
        const std::string &cargo_sub_order_name,
        const int qty);
};