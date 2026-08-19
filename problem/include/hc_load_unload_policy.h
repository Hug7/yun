/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <string>

#include "hc_constraint.h"

/**
 * @brief hard constraint for load unload policy about FILO
 */
class HcLoadUnloadPolicyFILO : public HardConstraint {
 public:
  HcLoadUnloadPolicyFILO() : HardConstraint("HcLoadUnloadPolicyFILO", 0, true, false) {};

  /**
   * @brief calculate the score of the hard constraint
   */
  HardConstrScore::UPtr eval(Load* load) override;
};

/**
 * @brief hard constraint for load unload policy about FIFO
 */
class HcLoadUnloadPolicyFIFO : public HardConstraint {
 public:
  HcLoadUnloadPolicyFIFO() : HardConstraint("HcLoadUnloadPolicyFIFO", 0, true, false) {};

  /**
   * @brief calculate the score of the hard constraint
   */
  HardConstrScore::UPtr eval(Load* load) override;
};
