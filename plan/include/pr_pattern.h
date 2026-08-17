/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <memory>

#include "dm_load.h"
#include "dm_node.h"
#include "dm_order.h"
#include "se_scenario.h"

class PickDropPattern {
 public:
  const Scenario* scenario;

  virtual Node* find_first_pick_node(std::unique_ptr<Node>& node);

  virtual Node* find_last_pick_node(std::unique_ptr<Node>& node);

  virtual Node* find_first_drop_node(std::unique_ptr<Node>& node);

  virtual Node* find_last_drop_node(std::unique_ptr<Node>& node);

  virtual Load* create_load(const Scenario* scenario) = 0;

  virtual bool add_order(Load* load, Order* order) = 0;

  PickDropPattern(const Scenario* scenario) : scenario(scenario) {}
};

class SPMD : public PickDropPattern {
 public:
  SPMD(const Scenario* scenario) : PickDropPattern(scenario) {}

  Load* create_load(const Scenario* scenario) override;

  bool add_order(Load* load, Order* order) override;
};
