/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <magic_enum/magic_enum.hpp>
#include <string>
#include <vector>

#include "algo_ls_move.h"

class Solution;
class LsNeighborhood;

/**
 * @brief 邻域作用范围：同一车次内部 / 跨车次
 */
enum class LsRouteScope {
  Intra,
  Inter,
};

/**
 * @brief 邻域算子编码（magic_enum 名字即 Lua code）
 */
enum class LsOperatorCode {
  LsIntraRouteRelocate,
  LsInterRouteRelocate,
  LsIntraRouteExchange,
  LsInterRouteExchange,
  LsTwoOpt,
  LsTwoOptStar,
};

/**
 * @brief 算子只读上下文
 * @details 不提供 Problem，算子无法自行判断可行性；只产候选数据。
 */
struct LsOperatorContext {
  const Solution* solution{nullptr};
  const LsNeighborhood* neighborhood{nullptr};
};

/**
 * @brief 邻域算子父类
 */
class LsOperator {
 public:
  /**
   * @brief 算子编码字符串，magic_enum 名字即 Lua code
   */
  const std::string code;

  explicit LsOperator(const LsOperatorCode operator_code)
      : code(std::string(magic_enum::enum_name(operator_code))) {}

  virtual ~LsOperator() = default;

  /**
   * @brief 枚举该算子在当前解上的全部候选；不访问 Load，只产数据
   */
  virtual void enumerate(const LsOperatorContext& context, std::vector<LsMove>& out) const = 0;

  /**
   * @brief 形如 `min_chain_len=2, max_chain_len=3`；无参数返回空串
   */
  [[nodiscard]] virtual std::string format_parameters() const = 0;
};
