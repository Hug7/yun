/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#include "pr_policy.h"

#include <format>
#include <magic_enum/magic_enum.hpp>

// ====== implement of PolicyFILO ======

// ====== implement of PolicyFIFO ======

// ====== implement of PolicyNone ======

// ====== implement of PolicyNone ======
LoadUnloadPolicy* LoadUnloadPolicyFactory::create_policy(LoadUnloadPolicyType policy_type) {
  switch (policy_type) {
    case LoadUnloadPolicyType::FIFO:
      return new PolicyFIFO();
    case LoadUnloadPolicyType::FILO:
      return new PolicyFILO();
    case LoadUnloadPolicyType::NONE:
      return new PolicyNone();
    default:
      throw std::invalid_argument(
          std::format("unknown policy type {}", magic_enum::enum_name(policy_type)));
  }
}
