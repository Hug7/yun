/**
* Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <string>

#include "pdm_parameter.h"
#include "prob_problem.h"
#include "se_scenario.h"
#include "visual_manager.h"

/**
 * @brief solver上下文
 */
class SolverContext {
public:
  /**
   * @brief 文件根目录
   */
  const std::string root_dir;
  /**
   * @brief 请求ID
   */
  const std::string request_id;
  /**
   * @brief 场景
   */
  Scenario* scenario;
  /**
   * @brief 参数
   */
  Parameter* parameter;
  /**
   * @brief 可视化管理器
   */
  VisualManager* visual_manager;
  /**
   * @brief 问题
   */
  Problem* problem;

  explicit SolverContext(std::string root_dir);

  ~SolverContext();
};
