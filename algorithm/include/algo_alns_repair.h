/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <magic_enum/magic_enum.hpp>
#include <string>

#include "pdm_context.h"
#include "pdm_solution.h"

/**
 * @brief alns repair算子编码
 */
enum class AlnsRepairOperatorCode {
  RepairGreedy,
};

constexpr AlnsRepairOperatorCode parse_alns_repair_operator_code(const std::string& code) {
  if (code == "RepairGreedy") {
    return AlnsRepairOperatorCode::RepairGreedy;
  } else {
    throw std::invalid_argument("Invalid AlnsRepairOperatorCode = " + code);
  }
}

/**
 * @brief alns repair算子父类
 */
class AlnsRepairOperator {
 public:
  /**
   * @brief repair算子编码
   */
  const std::string code;
  /**
   * @brief solver上下文
   */
  const SolverContext* context;

  virtual ~AlnsRepairOperator() = default;

  explicit AlnsRepairOperator(const AlnsRepairOperatorCode operator_code,
                              const SolverContext* _context)
      : code(std::string(magic_enum::enum_name(operator_code))), context(_context) {}

  virtual void call(Solution* sol) = 0;
};