/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "bd_common.h"
#include "bd_dimension.h"
#include "bd_dist_matrix.h"
#include "bd_label.h"

/**
 * @brief 车型类
 */
class VehicleModel : public Item {
 public:
  /**
   * @brief 距离矩阵编码
   */
  const DistMatrixCode* dist_matrix_code;
  /**
   * @brief 距离矩阵
   */
  const DistMatrix* dist_matrix;
  /**
   * @brief 车型属性值列表
   */
  std::vector<long> dim_vals;
  /**
   * @brief 车型标签集合值
   */
  LabelsetValue* labelset_value;
  /**
   * @brief 车型标签集合值位图
   */
  std::unique_ptr<LabelsetValueBitset> labelset_value_bitset;

  VehicleModel(const std::string& code, const std::string& name, const int ind,
               const DistMatrixCode* dist_matrix_code, const DistMatrix* dist_matrix,
               std::vector<long>& dim_vals, LabelsetValue* labelset_value,
               std::unique_ptr<LabelsetValueBitset> labelset_value_bitset)
      : Item(code, name, ind),
        dist_matrix_code(dist_matrix_code),
        dist_matrix(dist_matrix),
        dim_vals(dim_vals),
        labelset_value(labelset_value),
        labelset_value_bitset(std::move(labelset_value_bitset)) {}

  ~VehicleModel();

  /**
   * @brief update value of specified dimension
   * @param dim dimension
   * @param dim_val value of dimension
   */
  void update_dim_value(Dimension* dim, const double dim_val);

  /**
   * @brief Update labelset value and labelset value bitset
   * @param label_ind4labelset index of label in labelset
   * @param label_value label value
   */
  void update_labelset_value(const int label_ind4labelset, LabelValue* label_value);
};

/**
 * @brief 车型管理类
 */
class VehicleModelManager {
 public:
  /**
   * @brief 车型列表
   */
  std::vector<VehicleModel*> vehicle_models;
  /**
   * @brief 车型编码映射
   */
  std::unordered_map<std::string, VehicleModel*> vehicle_model_map;
  /**
   * @brief 车型索引生成器
   */
  std::unique_ptr<GenerateIndex> generate_index;
  /**
   * @brief 维度属性
   */
  DimensionManager* dim_manager;
  /**
   * @brief 标签集合
   */
  Labelset* labelset;
  /**
   * @brief 车型数量
   */
  int len;

  VehicleModelManager(DimensionManager* dim_manager, Labelset* labelset)
      : vehicle_models(),
        vehicle_model_map(),
        generate_index(std::make_unique<GenerateIndex>()),
        dim_manager(dim_manager),
        labelset(labelset),
        len(0) {};

  ~VehicleModelManager();

  /**
   * @brief 创建车型
   * @param code 车型编码
   * @param name 车型名称
   * @param dist_matrix_code 车型距离矩阵编码
   * @param dist_matrix 车型距离矩阵
   * @return 车型对象指针，若车型已存在则返回nullptr
   */
  VehicleModel* create_vehicle_model(const std::string& code, const std::string& name,
                                     const DistMatrixCode* dist_matrix_code,
                                     const DistMatrix* dist_matrix);

  /**
   * @brief 获取车型
   * @param code 车型编码
   * @return 车型对象指针，若不存在则返回nullptr
   */
  VehicleModel* get_vehicle_model(const std::string& code);
};