/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "pdm_load.h"
#include "pdm_order.h"
#include "pr_pattern.h"

class LoadUnloadPolicy {
 public:
  const PickDropPattern* pattern;

  LoadUnloadPolicy(const PickDropPattern* pattern) : pattern(pattern) {}

  virtual bool add_order(Load* load, const Order* order) = 0;
};

class PolicyFILO : public LoadUnloadPolicy {
 public:
  PolicyFILO(const PickDropPattern* pattern) : LoadUnloadPolicy(pattern) {}

  bool add_order(Load* load, const Order* order) override;
};

class PolicyFIFO : public LoadUnloadPolicy {
 public:
  PolicyFIFO(const PickDropPattern* pattern) : LoadUnloadPolicy(pattern) {}

  bool add_order(Load* load, const Order* order) override;
};

class PolicyNone : public LoadUnloadPolicy {
 public:
  PolicyNone(const PickDropPattern* pattern) : LoadUnloadPolicy(pattern) {}

  bool add_order(Load* load, const Order* order) override;
};
