/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <cstddef>
#include <cstdint>
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
 * @details 值类型位图。长度不超过 64 时所有位内联存放在对象里，不产生堆分配；
 * 长度超过 64 时才退化到堆上的 word 数组。长度没有硬上限。
 */
class Bitset {
 private:
  /**
   * @brief 内联存放的第一个 word，n_ <= 64 时使用
   */
  uint64_t inline_ = 0;
  /**
   * @brief n_ > 64 时使用的 word 数组
   */
  std::vector<uint64_t> overflow_;
  /**
   * @brief 位图的大小
   */
  size_t n_ = 0;

  [[nodiscard]] size_t words_len() const;

  [[nodiscard]] uint64_t* words();

  [[nodiscard]] const uint64_t* words() const;

 public:
  /**
   * @brief 构造函数
   * @param n 位图的大小
   */
  explicit Bitset(size_t n = 0);

  Bitset(const Bitset&) = default;

  Bitset(Bitset&& other) noexcept;

  Bitset& operator=(const Bitset&) = default;

  Bitset& operator=(Bitset&& other) noexcept;

  ~Bitset() = default;

  /**
   * @brief 位图长度
   */
  [[nodiscard]] size_t len() const { return n_; }

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
   * @param o 另一个位图，长度必须与本位图相同
   */
  void call_union(const Bitset& o);

  /**
   * @brief 取交集
   * @param o 另一个位图，长度必须与本位图相同
   */
  void call_intersection(const Bitset& o);

  /**
   * @brief 计算位图中1的个数
   */
  [[nodiscard]] int sum() const;
};

namespace BitsetUtils {

Bitset call_intersection(const Bitset& a, const Bitset& b);

Bitset call_union(const Bitset& a, const Bitset& b);

}  // namespace BitsetUtils
