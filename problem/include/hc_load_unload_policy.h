/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "hc_constraint.h"

/**
 * @brief 硬约束-装卸策略FILO
 */
class HcLoadUnloadPolicyFILO : public HardConstraint {
 public:
  HcLoadUnloadPolicyFILO()
      : HardConstraint(HardConstrCode::LoadUnloadPolicyFILO, 0, true, false) {}

  HardConstrScore::UPtr eval(Load* load) override;
};

/**
 * @brief 硬约束-装卸策略FIFO
 */
class HcLoadUnloadPolicyFIFO : public HardConstraint {
 public:
  HcLoadUnloadPolicyFIFO()
      : HardConstraint(HardConstrCode::LoadUnloadPolicyFIFO, 0, true, false) {}

  HardConstrScore::UPtr eval(Load* load) override;
};
