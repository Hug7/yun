/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "c_constant.h"

/**
 * @brief parameter configuration
 */
class Parameter {
 public:
  /**
   * @brief the type of pick and drop pattern
   */
  PickDropPatternType pick_drop_pattern{PickDropPatternType::SPMD};

  // constraints
  /**
   * @brief the number of drop nodes in the load
   */
  int max_pick_node_count{HardConstraintParameter::DEFAULT_MAX_PICK_NODE_COUNT};
  /**
   * @brief the number of drop nodes in the load
   */
  int max_drop_node_count{HardConstraintParameter::DEFAULT_MAX_DROP_NODE_COUNT};
  /**
   * @brief cost constraint "cc_dist" default distance factor
   */
  double cc_constr_dist_factor{CostConstraintParameter::CC_DIST_DEFAULT_DIST_FACTOR};

  Parameter() {};
};
