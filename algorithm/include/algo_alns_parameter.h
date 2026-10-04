/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <sol/sol.hpp>

/**
 * @brief ALNS 算法的参数在lua中的字段集合
 */
class AlnsParameterLuaField {
  public:
  static constexpr const char* MAX_ITER = "max_iter";
  static constexpr const char* MAX_SUB_ITER = "max_sub_iter";
  static constexpr const char* MAX_UNIMPROVED_ITER = "max_unimproved_iter";
  static constexpr const char* MAX_RUNNING_TIME_SEC = "max_running_time_sec";
  static constexpr const char* OPERATOR_LEARNING_FACTOR = "operator_learning_factor";
  static constexpr const char* OPERATOR_INITIAL_WEIGHT = "operator_initial_weight";
  static constexpr const char* INITIAL_TEMPERATURE = "initial_temperature";
  static constexpr const char* MIN_TEMPERATURE = "min_temperature";
  static constexpr const char* ANNEALING_FACTOR = "annealing_factor";
  static constexpr const char* DEFAULT_RANDOM_SEED = "default_random_seed";
  static constexpr const char* RUIN_OPERATOR_PARAMS = "ruin_operator_params";
  static constexpr const char* REPAIR_OPERATOR_PARAMS = "repair_operator_params";
  static constexpr const char* OPERATOR_CODE = "code";
  static constexpr const char* OPERATOR_PARAMS = "params";
};

/**
 * @brief ALNS 算法的参数
 */
class AlnsParameter {
 public:
  /**
   * @brief 最大迭代次数
   */
  int max_iter{200};
  /**
   * @brief 最大子迭代次数
   */
  int max_sub_iter{10};
  /**
   * @brief 最大无改进次数
   */
  int max_unimproved_iter{100};
  /**
   * @brief 最大运行时长
   */
  long max_running_time_sec{600};
  /**
   * @brief 算子学习因子
   */
  double operator_learning_factor{0.35};
  /**
   * @brief 算子初始权重
   */
  double operator_initial_weight{100.};
  /**
   * @brief 初始温度
   */
  double initial_temperature{10000.0};
  /**
   * @brief 最小温度
   */
  double min_temperature{0.97};
  /**
   * @brief 退火系数
   */
  double annealing_factor{0.97};
  /**
   * @brief 默认的随机种子
   */
  int default_random_seed{37};

  AlnsParameter() = default;

  ~AlnsParameter() = default;
};

/**
 * @brief 解析alns参数
 * @param cfg lua中定义的alns参数
 * @return
 */
AlnsParameter* parse_alns_parameter(const sol::table& cfg);

