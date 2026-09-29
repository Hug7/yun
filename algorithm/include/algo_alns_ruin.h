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
 * @brief alns ruin算子编码
 */
enum class AlnsRuinOperatorCode {
  RuinRandomLoad,
};

constexpr AlnsRuinOperatorCode parse_alns_ruin_operator_code(const std::string& code) {
  if (code == "RuinRandomLoad") {
    return AlnsRuinOperatorCode::RuinRandomLoad;
  } else {
    throw std::invalid_argument("Invalid AlnsRuinOperatorCode = " + code);
  }
}

/**
 * @brief alns ruin算子父类
 */
class AlnsRuinOperator {
 public:
  /**
   * @brief ruin算子编码
   */
  const std::string code;
  /**
   * @brief solver上下文
   */
  const SolverContext* context;

  explicit AlnsRuinOperator(const AlnsRuinOperatorCode operator_code, const SolverContext* _context)
      : code(std::string(magic_enum::enum_name(operator_code))), context(_context) {}

  virtual ~AlnsRuinOperator() = default;

  virtual void call(Solution* sol) = 0;
};