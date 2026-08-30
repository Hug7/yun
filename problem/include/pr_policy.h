/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "c_constant.h"
#include "pdm_load.h"
#include "pdm_order.h"
#include "pr_pattern.h"

class LoadUnloadPolicy {
 public:
  LoadUnloadPolicy() {}
};

class PolicyFILO : public LoadUnloadPolicy {
 public:
  PolicyFILO() : LoadUnloadPolicy() {}
};

class PolicyFIFO : public LoadUnloadPolicy {
 public:
  PolicyFIFO() : LoadUnloadPolicy() {}
};

class PolicyNone : public LoadUnloadPolicy {
 public:
  PolicyNone() : LoadUnloadPolicy() {}
};

namespace LoadUnloadPolicyFactory {
LoadUnloadPolicy* create_policy(LoadUnloadPolicyType policy_type);
} // namespace LoadUnloadPolicyFactory
