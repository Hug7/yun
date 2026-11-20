/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <string>
#include <vector>

#include "algo_ls_operator.h"

/**
 * @brief 搬 drop 节点链的参数
 * @details min_chain_len = max_chain_len = 1 即 relocate；2..3 即 or-opt。
 *          route_scope 由算子 code 决定，不通过 Lua 配置。
 */
struct LsSegmentRelocateParam {
  int min_chain_len{1};
  int max_chain_len{1};
  LsRouteScope route_scope{LsRouteScope::Intra};
};

/**
 * @brief relocate / or-opt 算子（共享实现，靠链长参数与作用范围区分）
 */
class LsSegmentRelocateOperator final : public LsOperator {
 public:
  /**
   * @brief 算子参数
   */
  const LsSegmentRelocateParam parameter;

  explicit LsSegmentRelocateOperator(const LsSegmentRelocateParam& _parameter);

  void enumerate(const LsOperatorContext& context, std::vector<LsMove>& out) const override;

  [[nodiscard]] std::string format_parameters() const override;
};
