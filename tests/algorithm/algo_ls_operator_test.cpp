/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#include "algo_ls_operator.h"

#include <gtest/gtest.h>

#include <cstddef>
#include <format>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <variant>
#include <vector>

#include "algo_ls.h"
#include "algo_ls_exchange.h"
#include "algo_ls_neighborhood.h"
#include "algo_ls_segment_relocate.h"
#include "algo_ls_two_opt.h"
#include "algo_ls_two_opt_star.h"
#include "pdm_solution.h"

namespace {
/**
 * @brief 只服务 2-opt* 枚举逻辑的邻域桩
 * @details 单元数由用例直接给出；PICK 站点共享关系退化为「其它所有车次」，
 *          用于验证算子自身的车次对与段长过滤
 */
class StubLsNeighborhood final : public LsNeighborhood {
 public:
  explicit StubLsNeighborhood(std::vector<int> unit_counts)
      : unit_counts_(std::move(unit_counts)) {}

  [[nodiscard]] int movable_unit_count(const Solution& /*sol*/, int load_ind) const override {
    return unit_counts_.at(static_cast<std::size_t>(load_ind));
  }

  [[nodiscard]] std::vector<std::vector<int>> load_unit_orders(const Solution& /*sol*/,
                                                               int /*load_ind*/) const override {
    return {};
  }

  [[nodiscard]] std::vector<int> candidate_target_loads(const Solution& /*sol*/,
                                                        const LsBlock& moved) const override {
    std::vector<int> targets;
    for (int load_ind = 0; load_ind < static_cast<int>(unit_counts_.size()); ++load_ind) {
      if (load_ind != moved.load_ind) {
        targets.push_back(load_ind);
      }
    }
    return targets;
  }

  [[nodiscard]] std::vector<int> target_slots(const Solution& /*sol*/, const LsBlock& /*moved*/,
                                              int /*to_load*/) const override {
    return {};
  }

  [[nodiscard]] Load* copy_load(const Load& /*load*/) const override { return nullptr; }

  bool apply(Load* /*from*/, Load* /*to*/, const LsMove& /*move*/) const override { return false; }

  void canonicalize(Load* /*load*/) const override {}

 private:
  std::vector<int> unit_counts_;
};

/**
 * @brief 桩不访问 Load 内容，指向 nullptr 只为确定车次数量
 */
void reset_loads(Solution& sol, const std::size_t load_count) {
  sol.loads.assign(load_count, nullptr);
}

[[nodiscard]] std::string format_exchange(const LsMove& move) {
  const LsExchangeMove& exchange = std::get<LsExchangeMove>(move);
  return std::format("{}:{}:{}-{}:{}:{}", exchange.first.load_ind, exchange.first.first_unit,
                     exchange.first.unit_count, exchange.second.load_ind,
                     exchange.second.first_unit, exchange.second.unit_count);
}
}  // namespace

TEST(LsOperatorTest, DefaultSetSplitsIntraAndInterRoute) {
  const std::vector<std::unique_ptr<LsOperator>> operators = create_ls_operators({});

  ASSERT_EQ(operators.size(), 7U);
  const std::vector<std::string> expected{
      "LsIntraRouteRelocate", "LsIntraRouteRelocate", "LsIntraRouteExchange",
      "LsInterRouteRelocate", "LsInterRouteRelocate", "LsInterRouteExchange",
      "LsTwoOptStar",
  };
  for (std::size_t ind = 0; ind < expected.size(); ++ind) {
    EXPECT_EQ(operators[ind]->code, expected[ind]) << "instance " << ind;
  }
}

TEST(LsOperatorTest, DefaultSetFormatsParameters) {
  const std::vector<std::unique_ptr<LsOperator>> operators = create_ls_operators({});

  ASSERT_EQ(operators.size(), 7U);
  EXPECT_EQ(operators[0]->format_parameters(), "min_chain_len=1, max_chain_len=1");
  EXPECT_EQ(operators[1]->format_parameters(), "min_chain_len=2, max_chain_len=3");
  EXPECT_EQ(operators[2]->format_parameters(), "");
  EXPECT_EQ(operators[3]->format_parameters(), "min_chain_len=1, max_chain_len=1");
  EXPECT_EQ(operators[4]->format_parameters(), "min_chain_len=2, max_chain_len=3");
  EXPECT_EQ(operators[5]->format_parameters(), "");
  EXPECT_EQ(operators[6]->format_parameters(), "min_tail_len=2, max_tail_len=0");
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

TEST(LsTwoOptStarOperatorTest, InvalidTailLengthThrows) {
  EXPECT_THROW((void)create_ls_operators({LsTwoOptStarParam{.min_tail_len = 0}}),
               std::invalid_argument);
  EXPECT_THROW((void)create_ls_operators({LsTwoOptStarParam{.min_tail_len = 3, .max_tail_len = 2}}),
               std::invalid_argument);
  EXPECT_NO_THROW((void)create_ls_operators({LsTwoOptStarParam{.min_tail_len = 1}}));
}

TEST(LsTwoOptStarOperatorTest, EnumeratesOnlyOrderedLoadPairsWithConfiguredTails) {
  Solution sol;
  reset_loads(sol, 2);
  const StubLsNeighborhood neighborhood({5, 4});
  const LsTwoOptStarOperator ls_operator(LsTwoOptStarParam{.min_tail_len = 2, .max_tail_len = 3});

  std::vector<LsMove> moves;
  ls_operator.enumerate(LsOperatorContext{.solution = &sol, .neighborhood = &neighborhood}, moves);

  // 只保留 first_load < second_load 的无序车次对，尾长全组合：2 个 × 2 个
  ASSERT_EQ(moves.size(), 4U);
  EXPECT_EQ(format_exchange(moves[0]), "0:3:2-1:2:2");
  EXPECT_EQ(format_exchange(moves[1]), "0:3:2-1:1:3");
  EXPECT_EQ(format_exchange(moves[2]), "0:2:3-1:2:2");
  EXPECT_EQ(format_exchange(moves[3]), "0:2:3-1:1:3");
}

TEST(LsTwoOptStarOperatorTest, UnboundedMaxTailCoversWholeRoute) {
  Solution sol;
  reset_loads(sol, 2);
  const StubLsNeighborhood neighborhood({5, 4});
  const LsTwoOptStarOperator ls_operator(LsTwoOptStarParam{.min_tail_len = 2, .max_tail_len = 0});

  std::vector<LsMove> moves;
  ls_operator.enumerate(LsOperatorContext{.solution = &sol, .neighborhood = &neighborhood}, moves);

  // 车次 0 尾长 2..5，车次 1 尾长 2..4
  EXPECT_EQ(moves.size(), 12U);
  EXPECT_EQ(format_exchange(moves.front()), "0:3:2-1:2:2");
  EXPECT_EQ(format_exchange(moves.back()), "0:0:5-1:0:4");
}

TEST(LsTwoOptStarOperatorTest, SkipsRoutesShorterThanMinTail) {
  Solution sol;
  reset_loads(sol, 2);
  const StubLsNeighborhood neighborhood({4, 1});
  const LsTwoOptStarOperator ls_operator(LsTwoOptStarParam{.min_tail_len = 2, .max_tail_len = 0});

  std::vector<LsMove> moves;
  ls_operator.enumerate(LsOperatorContext{.solution = &sol, .neighborhood = &neighborhood}, moves);

  EXPECT_TRUE(moves.empty());
}
