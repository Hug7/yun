/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <utility>
#include <vector>

#include "c_yaml_utils.h"
#include "hc_constraint_code.h"
#include "sc_constraint_code.h"

/**
 * @brief 硬约束项
 */
struct HardConstrItem {
  /**
   * @brief 硬约束编码
   */
  HardConstrCode code;
  /**
   * @brief 约束参数, 未声明 params 时为 null 节点
   */
  YamlNode params;

  explicit HardConstrItem(const HardConstrCode _code) : code(_code), params() {}

  explicit HardConstrItem(const HardConstrCode _code, YamlNode _params)
      : code(_code), params(std::move(_params)) {}
};

/**
 * @brief 软约束项
 */
struct SoftConstrItem {
  /**
   * @brief 软约束编码
   */
  SoftConstrCode code;
  /**
   * @brief 约束参数, 未声明 params 时为 null 节点
   */
  YamlNode params;

  explicit SoftConstrItem(const SoftConstrCode _code) : code(_code), params() {}

  explicit SoftConstrItem(const SoftConstrCode _code, YamlNode _params)
      : code(_code), params(std::move(_params)) {}
};

/**
 * @brief 解析硬约束项
 * @param yaml_nodes 硬约束参数列表
 * @return 硬约束项列表
 */
std::vector<HardConstrItem> parse_hard_constr_items(const YamlNode& yaml_nodes);

/**
 * @brief 解析软约束项
 * @param yaml_nodes 软约束参数列表
 * @return 软约束项列表
 */
std::vector<SoftConstrItem> parse_soft_constr_items(const YamlNode& yaml_nodes);