/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <string>
#include <vector>

#include "algo_ls_operator.h"

/**
 * @brief 交换两个块的参数
 * @details route_scope 由算子 code 决定，不通过 Lua 配置。
 */
struct LsExchangeParam {
  LsRouteScope route_scope{LsRouteScope::Intra};
};

/**
 * @brief exchange 算子
 */
class LsExchangeOperator final : public LsOperator {
 public:
  /**
   * @brief 算子参数
   */
  const LsExchangeParam parameter;

  explicit LsExchangeOperator(const LsExchangeParam& _parameter);

  void enumerate(const LsOperatorContext& context, std::vector<LsMove>& out) const override;

  [[nodiscard]] std::string format_parameters() const override;
};
