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

  virtual Load* create_load(LoadContext* context) const = 0;

  PickDropPattern() {}
};

class PatternSPMD : public PickDropPattern {
 public:
  PatternSPMD() : PickDropPattern() {}

  Load* create_load(LoadContext* context) const override;
};
