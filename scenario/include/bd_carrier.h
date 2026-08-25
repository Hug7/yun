/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "bd_available_vehicle.h"
#include "bd_common.h"
#include "bd_label.h"
#include "bd_vehicle.h"

/**
 * @brief 承运商类
 */
class Carrier : public Item {
 public:
  /**
   * @brief 承运商车辆列表
   */
  std::vector<Vehicle*> vehicles;
  /**
   * @brief 承运商车型编码映射
   */
  std::unordered_map<std::string, Vehicle*> vehicle_map;
  /**
   * @brief 承运商标签集值
   */
  LabelsetValue* labelset_value;
  /**
   * @brief 承运商标签集值位图
   */
  std::unique_ptr<LabelsetValueBitset> labelset_value_bitset;

  Carrier(const std::string& code, const std::string& name, const int ind,
          LabelsetValue* labelset_value, std::unique_ptr<LabelsetValueBitset> labelset_value_bitset)
      : Item(code, name, ind),
        vehicles(),
        vehicle_map(),
        labelset_value(labelset_value),
        labelset_value_bitset(std::move(labelset_value_bitset)) {}

  ~Carrier();

  /**
   * @brief 根据车型编码获取车辆
   * @param vehicle_model_code 车型编码
   * @return 车辆对象指针
   */
  Vehicle* get_vehicle(const std::string& vehicle_model_code);

  /**
   * @brief Update labelset value and labelset value bitset
   * @param label_ind4labelset index of label in labelset
   * @param label_value label value
   */
  void update_labelset_value(const int label_ind4labelset, LabelValue* label_value);
};

/**
 * @brief 承运商管理类
 */
class CarrierManager {
 private:
  /**
   * @brief 承运商索引生成器
   */
  std::unique_ptr<GenerateIndex> carrier_generate_index;
  /**
   * @brief 车辆索引生成器
   */
  std::unique_ptr<GenerateIndex> vehicle_generate_index;

 public:
  /**
   * @brief 承运商列表
   */
  std::vector<Carrier*> carriers;
  /**
   * @brief 承运商编码映射
   */
  std::unordered_map<std::string, Carrier*> carrier_map;
  /**
   * @brief 车辆列表
   */
  std::vector<Vehicle*> vehicles;
  /**
   * @brief 车辆数量
   */
  int vehicle_len;
  /**
   * @brief 承运商labelset
   */
  Labelset* labelset;

  CarrierManager(Labelset* labelset)
      : carrier_generate_index(std::make_unique<GenerateIndex>()),
        vehicle_generate_index(std::make_unique<GenerateIndex>()),
        carriers(),
        carrier_map(),
        vehicles(),
        vehicle_len(0),
        labelset(labelset) {};

  ~CarrierManager();

  /**
   * @brief 创建承运商
   * @param carrier 承运商对象
   */
  Carrier* create_carrier(const std::string& code, const std::string& name);

  /**
   * @brief 根据编码查找承运商
   * @param code 承运商编码
   * @return 承运商对象指针
   */
  Carrier* get_carrier(const std::string& code);

  /**
   * @brief 创建车辆
   * @param carrier_code 承运商编码
   * @param vehicle_model 车辆型号
   * @param count 数量
   * @param orig_loc 起始位置
   * @param dest_loc 目标位置
   * @return 车辆对象指针
   */
  Vehicle* create_vehicle(const std::string& carrier_code, const VehicleModel* vehicle_model,
                          const int count, const Location* orig_loc, const Location* dest_loc);

  /**
   * @brief 创建空的车辆位图
   * @return 车辆位图指针
   */
  Bitset::UPtr empty_vehicle_bitset();

  /**
   * @brief 创建全为真的车辆位图
   * @return 全为真的车辆位图指针
   */
  Bitset::UPtr full_vehicle_bitset();

  /**
   * @brief 创建空的可用车辆对象
   * @return 可用车辆对象指针
   */
  AvailableVehicle* empty_available_vehicle();

  /**
   * @brief 创建全为真的可用车辆对象
   * @return 可用车辆对象指针
   */
  AvailableVehicle* full_available_vehicle();
};
