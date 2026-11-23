/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <vector>

#include "hc_constraint.h"
#include "hc_label_value.h"
#include "hc_location.h"
#include "pdm_load.h"
#include "pdm_parameter.h"
#include "pdm_parameter_constraint.h"
#include "se_scenario.h"

/**
 * @brief 硬约束管理器
 */
class HardConstraintManager {
 public:
  /**
   * @brief 硬约束列表
   * @details 拥有指针的归属权
   */
  std::vector<HardConstraint*> constrs;
  /**
   * @brief 硬约束列表长度
   */
  int len{0};
  /**
   * @brief 硬约束预检开关
   * @details 由参数 hard_constr_precheck 控制, 默认关闭
   */
  bool precheck_enabled{false};

  explicit HardConstraintManager(const Scenario* scenario, const Parameter* parameter);

  ~HardConstraintManager();

  /**
   * @brief 添加硬约束到管理器中
   */
  void add_constr(HardConstraint* constr);

  /**
   * @brief 评价所有硬约束
   */
  void eval_constrs(Load* load, const LoadConstrProfile::UPtr& constr_profile) const;

  /**
   * @brief 预检所有硬约束
   * @details 按 priority 顺序短路: 任一约束判定为 kInfeasible 立即返回。
   * 只有全部约束都返回 kPass 时才返回 kPass, 否则返回 kUnknown。
   */
  [[nodiscard]] HcPrecheckVerdict precheck(Load* load, const Segment* segment,
                                           const VehicleResource& vehicle_resource) const;
};

namespace HardConstrParser {

HcMaxPickNodeCount* max_pick_node_count(const HardConstrItem& hard_constr_item);

HcMaxDropNodeCount* max_drop_node_count(const HardConstrItem& hard_constr_item);

HcLabelValueExclusive* label_value_exclusive(const Scenario* scenario,
                                             const HardConstrItem& hard_constr_item);

HcLabelValueCompatible* label_value_compatible(const Scenario* scenario,
                                               const HardConstrItem& hard_constr_item);

HcLabelValueMaxCount* label_value_max_count(const Scenario* scenario,
                                            const HardConstrItem& hard_constr_item);

}  // namespace HardConstrParser
