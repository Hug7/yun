/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "bdm_charge.h"
#include "cc_constraint.h"

/**
 * @brief 运输成本约束
 * @details 按计费表计算车次运费，写入 cost 约束评分
 */
class CcCharge : public CostConstraint {
 public:
  /**
   * @brief 计费管理器
   */
  const ChargeManager* charge_manager;

  explicit CcCharge(const ChargeManager* charge_manager)
      : CostConstraint(CostConstrCode::Charge, 0, true, true), charge_manager(charge_manager) {}

  /**
   * @brief 计算车次运费
   */
  CostConstrScore::UPtr eval(Load* load) override;
};
