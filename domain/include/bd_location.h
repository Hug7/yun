/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <memory>
#include <string>
#include <vector>

#include "bd_label.h"
#include "bd_work_plan.h"
#include "bd_predefine.h"

/**
 * @brief 位置类
 */
class Location : public Item {
 public:
  /**
   * @brief 纬度
   */
  const double lat;
  /**
   * @brief 经度
   */
  const double lng;
  /**
   * @brief 工作计划
   */
  WorkPlan* work_plan;
  /**
   * @brief 标签集合
   */
  LabelsetValue* labelset_value;
  /**
   * @brief 标签位图集合
   */
  std::unique_ptr<LabelsetValueBitset> labelset_value_bitset;
  /**
   * @brief 可用车辆
   */
  AvailableVehicle* available_vehicle;

  Location(const std::string& code, const std::string& name, const int ind, const double lat,
           const double lng, WorkPlan* work_plan, LabelsetValue* labelset_value,
           std::unique_ptr<LabelsetValueBitset> labelset_value_bitset)
      : Item(code, name, ind),
        lat(lat),
        lng(lng),
        work_plan(work_plan),
        labelset_value(labelset_value),
        labelset_value_bitset(std::move(labelset_value_bitset)) {}

  ~Location();

  /**
   * @brief 添加标签属性值到位置标签集合
   * @param label_ind 站点标签索引
   * @param label_value 标签属性值对象指针
   */
  void add_label_value(int label_ind, LabelValue* label_value);

  /**
   * @brief 设置可用车辆
   * @param available_vehicle 可用车辆集合对象指针
   */
  void set_available_vehicle(AvailableVehicle* available_vehicle);

  bool operator==(const Location* other) const noexcept { return ind == other->ind; }

  bool operator!=(const Location* other) const noexcept { return ind != other->ind; }
};

/**
 * @brief 位置管理类
 */
class LocationManager {
 private:
  /**
   * @brief 位置索引生成器
   */
  std::unique_ptr<GenerateIndex> generate_index;

 public:
  /**
   * @brief 位置列表
   */
  std::vector<Location*> locations;
  /**
   * @brief 位置编码映射
   */
  std::unordered_map<std::string, Location*> location_map;
  /**
   * @brief 位置标签集合
   */
  Labelset* labelset;
  /**
   * @brief 位置数量
   */
  int len;

  LocationManager(Labelset* labelset);

  ~LocationManager();

  /**
   * @brief 添加位置
   * @param code 位置编码
   * @param name 位置名称
   * @param lat 纬度
   * @param lng 经度
   * @return 位置对象指针，若已存在则抛出异常
   */
  Location* create_location(const std::string& code, const std::string& name, const double lat,
                            const double lng);

  /**
   * @brief 获取位置对象指针
   * @param code 位置编码
   * @return 位置对象指针，若不存在则返回nullptr
   */
  Location* get_location(const std::string& code);

  /**
   * @brief 获取默认位置对象指针
   */
  Location* get_default_location();
};
