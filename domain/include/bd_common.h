/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <cstdint>
#include <vector>
#include <string>
#include <memory>

class Item
{
public:
    /**
     * @brief 编码
     */
    const std::string code;
    /**
     * @brief 名称
     */
    const std::string name;
    /**
     * @brief 索引
     */
    const int ind;

    Item(const std::string &code, const std::string &name, const int ind) : code(code), name(name), ind(ind) {}

    ~Item() = default;
};



/**
 * @brief 生成唯一的索引值
 */
class GenerateIndex
{
private:
    /**
     * @brief 当前索引值
     */
    int ind;

public:
    GenerateIndex() : ind(-1) {};

    GenerateIndex(int start) : ind(start) {};

    ~GenerateIndex() = default;

    /**
     * @brief 获取下一个索引值
     * @return 下一个索引值
     */
    int next();
};

/**
 * @brief 位图类
 * @details 位图类用于存储一组布尔值，支持按位操作
 */
class Bitset
{
    /**
     * @brief 位图存储的数组
     */
    std::vector<uint64_t> w;
    /**
     * @brief 位图的大小
     */
    size_t n;

public:
    /**
     * @brief 构造函数
     * @param n 位图的大小
     */
    Bitset(size_t n) : w((n + 63) / 64, 0), n(n) {}

    ~Bitset() = default;

    /**
     * @brief 设置位图中第u位为1
     * @param u 位图中第u位
     */
    void set(size_t u);

    /**
     * @brief 清除位图中第u位
     * @param u 位图中第u位
     */
    void clear(size_t u);

    /**
     * @brief 清除所有位
     */
    void clear_all();

    /**
     * @brief 测试位图中第u位是否为1
     * @param u 位图中第u位
     * @return 如果第u位为1则返回true，否则返回false
     */
    bool test(size_t u) const;

    /**
     * @brief 将所有位设置为1
     */
    void set_all();

    /**
     * @brief 取并集
     * @param o 另一个位图
     */
    void call_union(Bitset *o);

    /**
     * @brief 取交集
     * @param o 另一个位图
     */
    void call_intersection(Bitset *o);

    /**
     * @brief 计算位图中1的个数
     */
    int popcount() const;
};