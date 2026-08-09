/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <string>
#include <vector>
#include <array>
#include <unordered_map>
#include <memory>
#include <bitset>

#include "bd_common.h"

/**
 * @brief 标签集支持的实体项
 */
class LabelSetSupportItem
{
public:
    enum Item
    {
        Location,
        VehicleModel,
        Order,
        Carrier,
    };

    static constexpr std::array item_names{
        "Location",
        "VehicleModel",
        "Order",
        "Carrier",
    };

    static inline const std::unordered_map<std::string, Item> item_map = {
        {"Location", Location},
        {"VehicleModel", VehicleModel},
        {"Order", Order},
        {"Carrier", Carrier},
    };
};

/**
 * @brief 标签属性值类
 */
class LabelValue
{
public:
    /**
     * @brief 标签属性值
     */
    const std::string value;
    /**
     * @brief 标签属性值索引
     */
    const int ind;

    LabelValue(const std::string &value, const int ind) : value(value), ind(ind) {};

};

/**
 * @brief 标签集合值位图类
 */
class LabelValueBitset : public Bitset
{
public:
    LabelValueBitset(size_t n) : Bitset(n) {};
};

/**
 * @brief 标签类
 */
class Label : public Item
{
public:
    /**
     * @brief label属性索引生成器
     */
    const std::unique_ptr<GenerateIndex> generate_index;
    /**
     * @brief 标签属性值列表
     */
    std::vector<LabelValue *> values;
    /*
     * @brief 标签属性值索引映射
     */
    std::unordered_map<std::string, LabelValue *> value_map;
    /**
     * @brief 标签属性值数量
     */
    int len;

    Label(const std::string &code,
          const std::string &name,
          const int ind) : Item(code, name, ind),
                           generate_index(std::make_unique<GenerateIndex>()),
                           values(),
                           value_map(),
                           len(0) {};
                           
    ~Label();

    /**
     * @brief 添加标签属性值
     * @param value 标签属性值
     * @return 标签属性值索引
     */
    LabelValue *add_label_value(const std::string &value);

    /**
     * @brief 获取标签属性值
     * @param value 标签属性值
     * @return 标签属性值，若不存在则返回nullptr
     */
    LabelValue *get_label_value(const std::string &value);
};

/**
 * @brief 标签集合值类
 */
class LabelsetValue
{
public:
    /**
     * @brief 标签集合值位图
     */
    std::vector<std::unordered_map<int, LabelValue *>> label_values;

    LabelsetValue(size_t n) : label_values(std::vector<std::unordered_map<int, LabelValue *>>(n)) {};

    ~LabelsetValue();

    /**
     * @brief 添加标签属性值到标签集合值
     * @param label_ind4labelset 标签在labeset中的索引
     * @param label_value 标签属性值对象指针
     */
    void add_label_value(const int label_ind4labelset, LabelValue *label_value);

    /**
     * @brief 合并标签集合值
     * @param labelset_value 标签集合值对象指针
     */
    void merge(LabelsetValue *labelset_value);
};

/**
 * @brief 标签集合值位图类
 */
class LabelsetValueBitset
{
public:
    /**
     * @brief 标签集合值位图
     */
    std::vector<std::unique_ptr<LabelValueBitset>> bitsets;

    LabelsetValueBitset(size_t n) : bitsets(std::vector<std::unique_ptr<LabelValueBitset>>(n)) {};

    /**
     * @brief 添加标签属性值到标签集合值位图
     * @param label_ind 标签索引
     * @param label_value 标签属性值对象指针
     */
    void add_label_value(int label_ind, LabelValue *label_value);

    /**
     * @brief 合并标签集合值位图
     * @param labelset_value_bitset 标签集合值位图对象指针
     */
    void merge(std::unique_ptr<LabelsetValueBitset> &labelset_value_bitset);
};

/**
 * @brief 标签集合类
 * @details 用于管理某类实体的标签集合
 */
class Labelset
{
public:
    /**
     * @brief 标签列表
     */
    std::vector<Label *> labels;
    /**
     * @brief 标签编码和索引映射
     */
    std::unordered_map<std::string, int> label_ind_map;
    /**
     * @brief 标签数量
     */
    int len;

    Labelset() : labels(), label_ind_map(), len(0) {};

    ~Labelset();

    /**
     * @brief 添加标签
     * @param label 标签对象指针
     */
    void add_label(Label *label);

    /**
     * @brief 获取标签在labelset中的索引
     * @param code 标签编码
     * @return 标签索引
     */
    int get_label_ind(const std::string &code);

    /**
     * @brief 生成标签集合值类对象指针
     */
    LabelsetValue *empty_labelset_value();

    /**
     * @brief 生成标签集合值位图类对象指针
     */
    std::unique_ptr<LabelsetValueBitset> empty_labelset_value_bitset();
};

/**
 * @brief 标签管理类
 * @details 用于管理所有标签集合的标签
 */
class LabelManager
{
public:
    /**
     * @brief 标签列表
     */
    std::vector<Label *> labels;
    /**
     * @brief 标签编码映射
     */
    std::unordered_map<std::string, Label *> label_map;
    /**
     * @brief 标签属性索引生成器
     */
    const std::unique_ptr<GenerateIndex> generate_index;
    /**
     * @brief 位置标签集合
     */
    Labelset *location_labelset;
    /**
     * @brief 车型标签集合
     */
    Labelset *vehicle_model_labelset;
    /**
     * @brief 订单标签集合
     */
    Labelset *order_labelset;
    /**
     * @brief 承运商标签集合
     */
    Labelset *carrier_labelset;

    LabelManager() : labels(),
                     label_map(),
                     generate_index(std::make_unique<GenerateIndex>()),
                     location_labelset(new Labelset()),
                     vehicle_model_labelset(new Labelset()),
                     order_labelset(new Labelset()),
                     carrier_labelset(new Labelset()) {};

    ~LabelManager();

    /**
     * @brief 创建标签
     * @param code 标签编码
     * @param code 标签名称
     * @return 标签对象指针，若标签已存在则抛出异常
     */
    Label *create_label(const std::string &code, const std::string &name);

    /**
     * @brief 获取标签对象指针
     * @param code 标签编码
     * @return 标签对象指针，若标签不存在则返回nullptr
     */
    Label *get_label(const std::string &code);

    /**
     * @brief 添加标签到标签集合
     * @details 根据 labelset_item 将label添加到对应item的标签集合中
     * @param code 标签编码
     * @param labelset_item 标签集合项
     */
    void labelset_add_label(const std::string &code, const std::string &labelset_item);

};