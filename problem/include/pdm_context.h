/**
* Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <spdlog/spdlog.h>

#include <memory>
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
   * @brief 输出文件目录
   */
  std::string output_dir;
  /**
   * @brief 日志器：本次请求专属，输出到控制台、log_dir、output_dir 三处
   */
  std::shared_ptr<spdlog::logger> logger;
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

  /**
   * @brief 构造solver上下文
   * @param root_dir 文件根目录
   * @param log_dir 全局日志目录
   * @param log_level 日志等级
   */
  explicit SolverContext(std::string root_dir, const std::string& log_dir,
                         const std::string& log_level);

  ~SolverContext();
};
