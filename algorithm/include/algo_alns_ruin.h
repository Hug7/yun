/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <magic_enum/magic_enum.hpp>
#include <string>
#include <unordered_set>
#include <unordered_map>

#include "pdm_context.h"
#include "pdm_solution.h"

/**
 * @brief alns ruin算子编码
 */
enum class AlnsRuinOperatorCode {
  RuinRandomLoad,
};

constexpr AlnsRuinOperatorCode convert_alns_ruin_operator_code(const std::string& code) {
  if (code == "RuinRandomLoad") {
    return AlnsRuinOperatorCode::RuinRandomLoad;
  } else {
    throw std::invalid_argument("Invalid AlnsRuinOperatorCode = " + code);
  }
}

class AlnsRuinLoadActivity {
 public:
  using UPtr = std::unique_ptr<AlnsRuinLoadActivity>;
  /**
   * @brief load索引
   */
  const int load_ind;
  /**
   * @brief activity 列表
   * @details activity数量是偶数，因为是 order 的提卸动作
   */
  std::vector<Activity*> activities;

  explicit AlnsRuinLoadActivity(const int load_ind) : load_ind(load_ind), activities() {};
};

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

  virtual std::unordered_map<int, AlnsRuinLoadActivity::UPtr> execute(Solution* sol) = 0;

  void call(Solution* sol);
};