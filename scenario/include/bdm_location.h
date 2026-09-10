/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <memory>
#include <string>
#include <vector>

#include "bdm_dimension.h"
#include "bdm_label.h"
#include "bdm_predefine.h"
#include "bdm_work_plan.h"

/**
 * @brief 站点类
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

  explicit Location(const std::string& code, const std::string& name, const int ind,
                    const double lat, const double lng, WorkPlan* work_plan,
                    LabelsetValue* labelset_value,
                    std::unique_ptr<LabelsetValueBitset> labelset_value_bitset)
      : Item(code, name, ind),
        lat(lat),
        lng(lng),
        work_plan(work_plan),
        labelset_value(labelset_value),
        labelset_value_bitset(std::move(labelset_value_bitset)),
        available_vehicle(nullptr) {}

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
 * @brief 站点管理类
 */
class LocationManager {
 private:
  /**
   * @brief 站点索引生成器
   */
  std::unique_ptr<GenerateIndex> generate_index;

 public:
  /**
   * @brief 站点列表
   */
  std::vector<Location*> locations;
  /**
   * @brief 站点编码映射
   */
  std::unordered_map<std::string, Location*> location_map;
  /**
   * @brief 站点标签集合
   */
  Labelset* labelset;
  /**
   * @brief 维度管理器
   */
  const DimensionManager* dim_manager;
  /**
   * @brief 站点数量
   */
  int len;

  LocationManager(Labelset* labelset, const DimensionManager* dim_manager);

  ~LocationManager();

  /**
   * @brief 添加站点
   * @param code 站点编码
   * @param name 站点名称
   * @param lat 纬度
   * @param lng 经度
   * @return 站点对象指针，若已存在则抛出异常
   */
  Location* create_location(const std::string& code, const std::string& name, const double lat,
                            const double lng);

  /**
   * @brief 获取站点对象指针
   * @param code 站点编码
   * @return 站点对象指针，若不存在则返回nullptr
   */
  Location* get_location(const std::string& code);

  /**
   * @brief 获取默认站点对象指针
   */
  Location* get_default_location();
};
