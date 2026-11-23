/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <array>
#include <memory>
#include <span>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "bdm_common.h"

/**
 * @brief 标签集支持的实体项
 */
class LabelSetSupportItem {
 public:
  enum Item {
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
class LabelValue {
 public:
  /**
   * @brief 标签属性值
   */
  const std::string value;
  /**
   * @brief 标签属性值索引
   */
  const int ind;

  LabelValue(std::string value, const int ind) : value(std::move(value)), ind(ind) {};
};

class Labelset;

/**
 * @brief 标签类
 */
class Label : public Item {
 public:
  /**
   * @brief label属性索引生成器
   */
  const std::unique_ptr<GenerateIndex> generate_index;
  /**
   * @brief 标签属性值列表
   */
  std::vector<LabelValue*> values;
  /*
   * @brief 标签属性值索引映射
   */
  std::unordered_map<std::string, LabelValue*> value_map;
  /**
   * @brief 标签属性值数量
   */
  int len;

  Label(const std::string& code, const std::string& name, const int ind)
      : Item(code, name, ind),
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
  LabelValue* add_label_value(const std::string& value);

  /**
   * @brief 获取标签属性值
   * @param value 标签属性值
   * @return 标签属性值，若不存在则返回nullptr
   */
  [[nodiscard]] LabelValue* get_label_value(const std::string& value) const;
};

/**
 * @brief 标签集合值类
 */
class LabelsetValue {
 public:
  /**
   * @brief 标签集合值
   */
  std::vector<std::unordered_map<int, LabelValue*>> label_values;

  explicit LabelsetValue(const size_t n)
      : label_values(std::vector<std::unordered_map<int, LabelValue*>>(n)) {};

  ~LabelsetValue();

  /**
   * @brief 添加标签属性值到标签集合值
   * @param label_ind4labelset 标签在labeset中的索引
   * @param label_value 标签属性值对象指针
   */
  void add_label_value(int label_ind4labelset, LabelValue* label_value);

  /**
   * @brief 合并标签集合值
   * @param labelset_value 标签集合值对象指针
   */
  void merge(LabelsetValue* labelset_value);
};

/**
 * @brief 标签集合值位图类
 * @details 所有标签的位连续存放在同一个 word 数组里（arena），一个 labelset 的位图
 * 只有一块内存；每个标签的 word 偏移由 Labelset 统一维护。
 */
class LabelsetValueBitset {
 public:
  using UPtr = std::unique_ptr<LabelsetValueBitset>;

 private:
  /**
   * @brief 位图布局所属的标签集合
   */
  const Labelset* labelset_;
  /**
   * @brief 所有标签的位，按 Labelset 中的标签顺序连续存放
   */
  std::vector<uint64_t> words_;

 public:
  explicit LabelsetValueBitset(const Labelset* labelset);

  [[nodiscard]] const Labelset* labelset() const { return this->labelset_; }

  /**
   * @brief 获取某标签在 arena 中的位图视图
   * @param label_ind 标签索引
   * @return 该标签对应的连续 word 区间，按标签内取值索引从低到高排列
   */
  [[nodiscard]] std::span<const uint64_t> label_words(int label_ind) const;

  /**
   * @brief 添加标签属性值到标签集合值位图
   * @param label_ind 标签索引
   * @param label_value 标签属性值对象指针
   */
  void add_label_value(int label_ind, LabelValue* label_value);

  /**
   * @brief 测试某标签的某取值位是否为1
   * @param label_ind 标签索引
   * @param value_ind 标签属性值索引
   */
  [[nodiscard]] bool test(int label_ind, int value_ind) const;

  /**
   * @brief 设置某标签的某取值位为1
   * @param label_ind 标签索引
   * @param value_ind 标签属性值索引
   */
  void set(int label_ind, int value_ind);

  /**
   * @brief 统计某标签中被置位的取值数量
   * @param label_ind 标签索引
   */
  [[nodiscard]] int sum(int label_ind) const;

  /**
   * @brief 合并标签集合值位图
   * @param other 标签集合值位图，布局必须与本位图相同
   */
  void merge(const LabelsetValueBitset& other);

  /**
   * @brief 重置标签集合值位图
   */
  void clear_bitsets();
};

/**
 * @brief 标签集合类
 * @details 用于管理某类实体的标签集合
 */
class Labelset {
 public:
  /**
   * @brief 标签列表
   */
  std::vector<Label*> labels;
  /**
   * @brief 标签编码和索引映射
   */
  std::unordered_map<std::string, int> label_ind_map;
  /**
   * @brief 标签数量
   */
  int len;

 private:
  /**
   * @brief 每个标签在 arena 中的起始 word 偏移，长度 len + 1，最后一项是总 word 数
   */
  mutable std::vector<uint32_t> label_word_offsets_;
  /**
   * @brief arena 的总 word 数
   */
  mutable size_t words_len_ = 0;
  /**
   * @brief 布局是否已经生成
   */
  mutable bool layout_built_ = false;

  /**
   * @brief 按当前的标签和取值数量生成 arena 布局
   * @details 必须在所有 LabelValue 加载完成后调用；布局生成后不再支持追加 LabelValue
   */
  void build_layout() const;

 public:
  explicit Labelset() : labels(), label_ind_map(), len(0) {};

  /**
   * @brief arena 的总 word 数
   */
  [[nodiscard]] size_t labelset_words_len() const;

  /**
   * @brief 某标签在 arena 中的起始 word 偏移
   * @param label_ind 标签索引
   */
  [[nodiscard]] size_t label_word_offset(int label_ind) const;

  ~Labelset();

  /**
   * @brief 添加标签
   * @param label 标签对象指针
   */
  void add_label(Label* label);

  /**
   * @brief 判断labelset是否包含`标签编码`
   * @param code 标签编码
   * @return 是否存在
   */
  bool contains_label_code(const std::string& code) const;

  /**
   * @brief 获取标签在labelset中的索引
   * @param code 标签编码
   * @return 标签索引
   */
  int get_label_ind(const std::string& code) const;

  /**
   * @brief 根据标签编码获取label
   * @param code 标签编码
   * @return 标签
   */
  Label* get_label(const std::string& code) const;

  /**
   * @brief 生成标签集合值类对象指针
   */
  LabelsetValue* empty_labelset_value() const;

  /**
   * @brief 生成标签集合值位图类对象指针
   */
  LabelsetValueBitset::UPtr empty_labelset_value_bitset() const;
};

/**
 * @brief 标签管理类
 * @details 用于管理所有标签集合的标签
 */
class LabelManager {
 public:
  /**
   * @brief 标签列表
   */
  std::vector<Label*> labels;
  /**
   * @brief 标签编码映射
   */
  std::unordered_map<std::string, Label*> label_map;
  /**
   * @brief 标签属性索引生成器
   */
  const std::unique_ptr<GenerateIndex> generate_index;
  /**
   * @brief 位置标签集合
   */
  Labelset* location_labelset;
  /**
   * @brief 车型标签集合
   */
  Labelset* vehicle_model_labelset;
  /**
   * @brief 订单标签集合
   */
  Labelset* order_labelset;
  /**
   * @brief 承运商标签集合
   */
  Labelset* carrier_labelset;

  LabelManager()
      : labels(),
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
  Label* create_label(const std::string& code, const std::string& name);

  /**
   * @brief 获取标签对象指针
   * @param code 标签编码
   * @return 标签对象指针，若标签不存在则返回nullptr
   */
  Label* get_label(const std::string& code);

  /**
   * @brief 添加标签到标签集合
   * @details 根据 labelset_item 将label添加到对应item的标签集合中
   * @param code 标签编码
   * @param labelset_item 标签集合项
   */
  void labelset_add_label(const std::string& code, const std::string& labelset_item);
};
