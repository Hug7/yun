/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>

#include "bd_common.h"

/**
 * @brief 精度的幂
 * @details 0:10^0:1, 1:10^1:10, 2:10^2:100, 3:10^3:1000, 4:10^4:10000
 */
constexpr int PRECISION_POW[5] = {1, 10, 100, 1000, 10000};

/**
 * @brief 计算维度
 */
class Dimension
{
public:
    /**
     * @brief 维度编码
     */
    const std::string code;
    /**
     * @brief 维度名称
     */
    const std::string name;
    /**
     * @brief 维度索引
     */
    const int ind;
    /**
     * @brief 维度属性的精度
     * @details 维度属性值的精度，例如: 0表示整数，1表示小数点后一位，2表示小数点后两位; 计算时会将属性值乘以10的precision次方进行处理
     */
    const int precision;

    Dimension(const std::string &code,
              const std::string &name,
              int ind,
              int precision) : code(code),
                               name(name),
                               ind(ind),
                               precision(precision) {};

    ~Dimension() = default;
};

/**
 * @brief 计算维度管理器
 */
class DimensionManager
{
public:
    /**
     * @brief 维度列表
     */
    std::vector<Dimension *> dimensions;
    /**
     * @brief 维度编码映射
     */
    std::unordered_map<std::string, Dimension *> dimension_map;
    /**
     * @brief 维度索引生成器
     */
    std::unique_ptr<GenerateIndex> generate_index;
    /**
     * @brief 维度数量
     */
    int len;

    DimensionManager() : dimensions(),
                         dimension_map(),
                         generate_index(std::make_unique<GenerateIndex>()),
                         len(0) {};

    ~DimensionManager();

    /**
     * @brief 添加计算维度
     * @param code 维度编码
     * @param name 维度名称
     * @param precision 维度精度
     * @return 维度对象指针，若维度已存在则返回nullptr
     */
    Dimension *create_dimension(const std::string &code, const std::string &name, const int precision);

    /**
     * @brief 获取维度对象指针
     * @param code 维度编码
     * @return 维度对象指针，若不存在则返回nullptr
     */
    Dimension *get_dimension(std::string code);

    /**
     * @brief 获取所有维度的空数组
     */
    std::vector<long> empty_dim_values();
};