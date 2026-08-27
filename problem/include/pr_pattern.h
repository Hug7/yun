/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <memory>

#include "dm_load.h"
#include "dm_node.h"
#include "dm_order.h"

class PickDropPattern {
 public:

  virtual Node* find_first_pick_node(std::unique_ptr<Node>& node);

  virtual Node* find_last_pick_node(std::unique_ptr<Node>& node);

  virtual Node* find_first_drop_node(std::unique_ptr<Node>& node);

  virtual Node* find_last_drop_node(std::unique_ptr<Node>& node);

  virtual Load* create_load(LoadContext* context) = 0;

  virtual bool add_order(Load* load, Order* order) = 0;

  PickDropPattern() {}
};

class SPMD : public PickDropPattern {
 public:
  SPMD() : PickDropPattern() {}

  Load* create_load(LoadContext* context) override;

  bool add_order(Load* load, Order* order) override;
};
