/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#include "algo_ls_operator.h"

#include <gtest/gtest.h>

#include <cstddef>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include "algo_ls.h"
#include "algo_ls_exchange.h"
#include "algo_ls_segment_relocate.h"
#include "algo_ls_two_opt.h"

TEST(LsOperatorTest, DefaultSetSplitsIntraAndInterRoute) {
  const std::vector<std::unique_ptr<LsOperator>> operators = create_ls_operators({});

  ASSERT_EQ(operators.size(), 6U);
  const std::vector<std::string> expected{
      "LsIntraRouteRelocate", "LsIntraRouteRelocate", "LsIntraRouteExchange",
      "LsInterRouteRelocate", "LsInterRouteRelocate", "LsInterRouteExchange",
  };
  for (std::size_t ind = 0; ind < expected.size(); ++ind) {
    EXPECT_EQ(operators[ind]->code, expected[ind]) << "instance " << ind;
  }
}

TEST(LsOperatorTest, DefaultSetFormatsParameters) {
  const std::vector<std::unique_ptr<LsOperator>> operators = create_ls_operators({});

  ASSERT_EQ(operators.size(), 6U);
  EXPECT_EQ(operators[0]->format_parameters(), "min_chain_len=1, max_chain_len=1");
  EXPECT_EQ(operators[1]->format_parameters(), "min_chain_len=2, max_chain_len=3");
  EXPECT_EQ(operators[2]->format_parameters(), "");
  EXPECT_EQ(operators[3]->format_parameters(), "min_chain_len=1, max_chain_len=1");
  EXPECT_EQ(operators[4]->format_parameters(), "min_chain_len=2, max_chain_len=3");
  EXPECT_EQ(operators[5]->format_parameters(), "");
}

TEST(LsOperatorTest, RouteScopeSelectsOperatorCode) {
  const std::vector<std::unique_ptr<LsOperator>> inter_exchange =
      create_ls_operators({LsExchangeParam{.route_scope = LsRouteScope::Inter}});
  ASSERT_EQ(inter_exchange.size(), 1U);
  EXPECT_EQ(inter_exchange[0]->code, "LsInterRouteExchange");

  const std::vector<std::unique_ptr<LsOperator>> inter_relocate =
      create_ls_operators({LsSegmentRelocateParam{
          .min_chain_len = 2, .max_chain_len = 3, .route_scope = LsRouteScope::Inter}});
  ASSERT_EQ(inter_relocate.size(), 1U);
  EXPECT_EQ(inter_relocate[0]->code, "LsInterRouteRelocate");
  EXPECT_EQ(inter_relocate[0]->format_parameters(), "min_chain_len=2, max_chain_len=3");
}

TEST(LsOperatorTest, InvalidChainLengthThrows) {
  EXPECT_THROW((void)create_ls_operators({LsSegmentRelocateParam{.min_chain_len = 0}}),
               std::invalid_argument);
  EXPECT_THROW(
      (void)create_ls_operators({LsSegmentRelocateParam{.min_chain_len = 3, .max_chain_len = 2}}),
      std::invalid_argument);
}

TEST(LsOperatorTest, InvalidSpanLengthThrows) {
  EXPECT_THROW((void)create_ls_operators({LsTwoOptParam{.min_span_len = 1}}),
               std::invalid_argument);
  EXPECT_THROW((void)create_ls_operators({LsTwoOptParam{.min_span_len = 3, .max_span_len = 2}}),
               std::invalid_argument);
}
