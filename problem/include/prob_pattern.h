/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <memory>

#include "pdm_load.h"
#include "pdm_node.h"

class PickDropPattern {
 public:

  virtual Node* find_first_pick_node(std::unique_ptr<Node>& node) const;

  virtual Node* find_last_pick_node(std::unique_ptr<Node>& node) const;

  virtual Node* find_first_drop_node(std::unique_ptr<Node>& node) const;

  virtual Node* find_last_drop_node(std::unique_ptr<Node>& node) const;

  virtual Node* find_load_first_pick_node(Load* load) const;

  virtual Node* find_load_last_pick_node(Load* load) const;

  virtual Node* find_load_first_drop_node(Load* load) const;

  virtual Node* find_load_last_drop_node(Load* load) const;

  virtual Load* create_load(LoadContext* context) const = 0;

  virtual bool insert_last_drop(Load* load, const Order* order) const = 0;
  
  PickDropPattern() = default;
};

class PatternSPMD : public PickDropPattern {
 public:
  PatternSPMD() : PickDropPattern() {}

  Node* find_load_first_pick_node(Load* load) const override;

  Node* find_load_last_pick_node(Load* load) const override;

  Node* find_load_first_drop_node(Load* load) const override;

  Node* find_load_last_drop_node(Load* load) const override;

  Load* create_load(LoadContext* context) const override;

  bool insert_last_drop(Load* load, const Order* order) const override;
};
