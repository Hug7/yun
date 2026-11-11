/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "cc_manager.h"
#include "hc_manager.h"
#include "pdm_load.h"
#include "pdm_parameter.h"
#include "prob_pattern.h"
#include "prob_policy.h"
#include "sc_manager.h"
#include "se_scenario.h"

/**
 * @brief 问题定义
 * @details 场景与参数下的一套约束与偏好：对一个 Load 求值，得到它是否可行、代价多少。
 */
class Problem {
 public:
  const Scenario* scenario;

  Parameter* parameter;

  LoadUnloadPolicy* lu_policy;

  PickDropPattern* pd_pattern;

  LoadContext* load_context;

  HardConstraintManager* hc_manager;

  SoftConstraintManager* sc_manager;

  CostConstraintManager* cc_manager;

  Problem(const Scenario* scenario, Parameter* parameter);

  ~Problem();

  /**
   * @brief 评价约束并写回 load->constr_profile
   * @details 结果落在 Load 自身上，供后续 is_feasible / obj_val 判断
   */
  void eval_load(Load* load) const;

  /**
   * @brief 评价约束并返回一份新的约束 profile，不写回 load
   * @details 用于试插、试换车型等"不落地"的评估；调用方持有返回的 profile
   */
  LoadConstrProfile::UPtr evaluate(Load* load) const;

 private:
  /**
   * @brief 评价约束流程
   * @details 分别被 eval_load 与 evaluate 使用，并且差别仅在于 profile 的归属
   */
  void eval_into(Load* load, const LoadConstrProfile::UPtr& constr_profile) const;
};
