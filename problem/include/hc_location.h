/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "hc_constraint.h"

/**
 * @brief 硬约束-最大提货节点数
 */
class HcMaxPickNodeCount : public HardConstraint {
 public:
  /**
   * @brief 最大提货节点数
   */
  const int max_pick_node_count;

  explicit HcMaxPickNodeCount(const int max_pick_node_count)
      : HardConstraint(HardConstrCode::MaxPickNodeCount, 0, false, false),
        max_pick_node_count(max_pick_node_count) {};

  HardConstrScore::UPtr eval(Load* load) override;
};

/**
 * @brief 硬约束-最大卸货节点数
 */
class HcMaxDropNodeCount : public HardConstraint {
 public:
  /**
   * @brief 最大卸货节点数
   */
  const int max_drop_node_count;

  explicit HcMaxDropNodeCount(const int max_drop_node_count)
      : HardConstraint(HardConstrCode::MaxDropNodeCount, 0, false, false),
        max_drop_node_count(max_drop_node_count) {};

  HardConstrScore::UPtr eval(Load* load) override;
};

/**
 * @brief 硬约束-可用车型
 */
class HcAvailableVehicle : public HardConstraint {
 public:
  HcAvailableVehicle() : HardConstraint(HardConstrCode::AvailableVehicle, 0, false, true) {};

  HardConstrScore::UPtr eval(Load* load) override;
};

/**
 * @brief 硬约束-时间窗
 */
class HcTimeWindow : public HardConstraint {
 public:
  HcTimeWindow() : HardConstraint(HardConstrCode::TimeWindow, 0, true, true) {};

  HardConstrScore::UPtr eval(Load* load) override;
};
