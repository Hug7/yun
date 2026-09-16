/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <memory>
#include <string>
#include <utility>
#include <vector>

class Item {
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

  Item(std::string code, std::string name, const int ind)
      : code(std::move(code)), name(std::move(name)), ind(ind) {}

  ~Item() = default;
};

/**
 * @brief 生成唯一的索引值
 */
class GenerateIndex {
 private:
  /**
   * @brief 当前索引值
   */
  int ind;

 public:
  GenerateIndex() : ind(-1) {};

  explicit GenerateIndex(int start) : ind(start) {};

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
class Bitset {
 private:
  /**
   * @brief 位图存储的数组
   */
  std::vector<uint64_t> w;
  /**
   * @brief 位图的大小
   */
  size_t n;

 public:
  using UPtr = std::unique_ptr<Bitset>;

  /**
   * @brief 构造函数
   * @param n 位图的大小
   */
  explicit Bitset(const size_t n) : w((n + 63) / 64, 0), n(n) {}

  explicit Bitset(const UPtr& other);

  ~Bitset() = default;

  /**
   * @brief 位图长度
   */
  [[nodiscard]] size_t len() const { return n; };

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
  [[nodiscard]] bool test(size_t u) const;

  /**
   * @brief 将所有位设置为1
   */
  void set_all();

  /**
   * @brief 取并集
   * @param o 另一个位图
   */
  void call_union(const Bitset* o);

  /**
   * @brief 取交集
   * @param o 另一个位图
   */
  void call_intersection(const Bitset* o);

  /**
   * @brief 取并集
   * @param o 另一个位图
   */
  void call_union(const Bitset::UPtr& o);

  /**
   * @brief 取交集
   * @param o 另一个位图
   */
  void call_intersection(const Bitset::UPtr& o);

  /**
   * @brief 计算位图中1的个数
   */
  [[nodiscard]] int sum() const;
};
