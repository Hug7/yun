/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <spdlog/spdlog.h>

#include <memory>
#include <string>
#include <vector>

#include "bd_common.h"
#include "c_constant.h"

/**
 * @brief 距离矩阵自定义code
 */
class DistMatrixCode : public Item {
 public:
  DistMatrixCode(const std::string& code, const std::string& name, const int ind)
      : Item(code, name, ind) {}

  bool operator==(const DistMatrixCode* other) const noexcept { return ind == other->ind; }

  bool operator!=(const DistMatrixCode* other) const noexcept { return ind != other->ind; }
};

struct DistTime {
  long dist = DistMatrixParameter::MAX_DISTANCE;
  long time = DistMatrixParameter::MAX_TIME;
};

/**
 * @brief 距离矩阵
 * @details 距离矩阵用于存储两个位置之间的距离和耗时
 */
class DistMatrix {
 private:
  /**
   * @brief 距离和耗时数组
   * @details 将二维数组展开为一维数组，方便存储和访问
   */
  DistTime* arr;

 public:
  /**
   * @brief dist matrix code index
   */
  const int dist_matrix_ind;
  /**
   * @brief lenght of the matrix
   */
  const int matrix_len;

  DistMatrix(const int dist_matrix_ind, const int matrix_len);

  ~DistMatrix();

  /**
   * @brief 获取两个位置之间的距离
   * @param from_ind 起始位置索引
   * @param to_ind 结束位置索引
   * @return long 两个位置之间的距离
   */
  long get_dist(const int from_ind, const int to_ind) const;

  /**
   * @brief 获取两个位置之间的耗时
   * @param from_ind 起始位置索引
   * @param to_ind 结束位置索引
   * @return long 两个位置之间的耗时
   */
  long get_time(const int from_ind, const int to_ind) const;

  /**
   * @brief 获取两个位置之间的距离和耗时
   * @param from_ind 起始位置索引
   * @param to_ind 结束位置索引
   * @return DistTime 两个位置之间的距离和耗时
   */
  DistTime get_dist_time(const int from_ind, const int to_ind) const;

  /**
   * @brief 设置两个位置之间的距离和耗时
   * @param from_ind 起始位置索引
   * @param to_ind 结束位置索引
   * @param dist 两个位置之间的距离, 单位:m
   * @param time 两个位置之间的耗时, 单位:s
   */
  void set_dist_time(const int from_ind, const int to_ind, const long dist, const long time);
};

/**
 * @brief 距离矩阵管理器
 * @details 距离矩阵管理器用于管理多个距离矩阵
 */
class DistMatrixManager {
 public:
  /**
   * @brief 距离矩阵长度
   */
  const int matrix_len;
  /**
   * @brief 距离矩阵自定义code映射表
   */
  std::unordered_map<std::string, DistMatrixCode*> dist_matrix_code_map;
  /**
   * @brief 距离矩阵编码索引生成器
   */
  std::unique_ptr<GenerateIndex> generate_index;
  /**
   * @brief 距离矩阵集合
   */
  std::vector<DistMatrix*> dist_matrixes;

  DistMatrixManager(const int matrix_len)
      : matrix_len(matrix_len),
        dist_matrix_code_map(),
        generate_index(std::make_unique<GenerateIndex>()),
        dist_matrixes() {};

  ~DistMatrixManager();

  /**
   * @brief 创建距离矩阵编码
   * @param dist_matrix_code 距离矩阵自定义code
   * @param dist_matrix_code_name 距离矩阵自定义code名称
   */
  void create_dist_matrix_code(const std::string& dist_matrix_code,
                               const std::string& dist_matrix_code_name);
  /**
   * @brief 获取距离矩阵编码
   * @param dist_matrix_code 距离矩阵自定义code
   */
  DistMatrixCode* get_dist_matrix_code(const std::string& dist_matrix_code);
  /**
   * @brief 设置距离矩阵的距离和耗时
   * @param dist_matrix_code 距离矩阵自定义code
   * @param row_ind 距离矩阵行索引
   * @param col_ind 距离矩阵列索引
   * @param cost_dist 两个位置之间的距离, 单位:m
   * @param cost_time 两个位置之间的耗时, 单位:s
   */
  void set_dist_time(const std::string& dist_matrix_code, const int row_ind, const int col_ind,
                     const long cost_dist, const long cost_time);
  /**
   * @brief 获取距离矩阵
   * @param dist_matrix_code 距离矩阵自定义code
   * @return DistMatrix* 距离矩阵
   */
  DistMatrix* get_dist_matrix(DistMatrixCode* dist_matrix_code);
};
