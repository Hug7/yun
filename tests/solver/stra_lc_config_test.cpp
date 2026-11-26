/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>

#include <memory>
#include <sol/sol.hpp>
#include <stdexcept>
#include <string>
#include <variant>
#include <vector>

#include "algo_ls.h"
#include "algo_ls_exchange.h"
#include "algo_ls_segment_relocate.h"
#include "algo_ls_two_opt.h"
#include "c_log.h"
#include "stra_ls_config.h"

namespace {

/**
 * @brief Lua→POD 解析测试夹具
 * @details logger 设为 off, 避免解析期间打印参数清单干扰测试输出
 */
class StraLcConfigTest : public ::testing::Test {
 protected:
  void SetUp() override {
    this->lua.open_libraries(sol::lib::base, sol::lib::table);
    this->logger = LogManager::create("stra_ls_config_test", "", "", "off");
  }

  [[nodiscard]] LsStrategyConfig parse(const sol::object& cfg) const {
    return parse_local_search_strategy_config(cfg, LsParameter{}, this->logger);
  }

  sol::state lua;
  std::shared_ptr<spdlog::logger> logger;
};

}  // namespace

TEST_F(StraLcConfigTest, NilConfigUsesStandaloneDefaults) {
  const sol::object nil_cfg = this->lua["not_defined"];

  const LsStrategyConfig config = this->parse(nil_cfg);

  EXPECT_DOUBLE_EQ(config.parameter.epsilon, LsParameter{}.epsilon);
  EXPECT_EQ(config.parameter.improvement, LsImprovement::Best);
  EXPECT_TRUE(config.operators.empty());
}

TEST_F(StraLcConfigTest, EmptyTableUsesStandaloneDefaults) {
  const sol::table cfg = this->lua.create_table();

  const LsStrategyConfig config = this->parse(cfg);

  EXPECT_DOUBLE_EQ(config.parameter.epsilon, LsParameter{}.epsilon);
  EXPECT_EQ(config.parameter.improvement, LsImprovement::Best);
  EXPECT_EQ(config.parameter.max_evaluated, 0);
  EXPECT_EQ(config.parameter.knn_neighbor_count, 10);
  EXPECT_TRUE(config.operators.empty());
}

TEST_F(StraLcConfigTest, MaxEvaluatedRoundTripsAndValidates) {
  this->lua.script(R"(
    unlimited_cfg = { max_evaluated = 0 }
    budget_cfg = { max_evaluated = 250 }
    negative_cfg = { max_evaluated = -1 }
    overflow_cfg = { max_evaluated = 1e18 }
    wrong_type_cfg = { max_evaluated = "many" }
  )");

  EXPECT_EQ(this->parse(this->lua["unlimited_cfg"]).parameter.max_evaluated, 0);
  EXPECT_EQ(this->parse(this->lua["budget_cfg"]).parameter.max_evaluated, 250);
  EXPECT_THROW((void)this->parse(this->lua["negative_cfg"]), std::invalid_argument);
  EXPECT_THROW((void)this->parse(this->lua["overflow_cfg"]), std::invalid_argument);
  EXPECT_THROW((void)this->parse(this->lua["wrong_type_cfg"]), std::invalid_argument);
}

TEST_F(StraLcConfigTest, KnnNeighborCountRoundTripsAndValidates) {
  this->lua.script(R"(
    default_cfg = {}
    small_cfg = { knn_neighbor_count = 3 }
    disabled_cfg = { knn_neighbor_count = 0 }
    negative_cfg = { knn_neighbor_count = -2 }
    overflow_cfg = { knn_neighbor_count = 1e18 }
    wrong_type_cfg = { knn_neighbor_count = "eight" }
  )");

  EXPECT_EQ(this->parse(this->lua["default_cfg"]).parameter.knn_neighbor_count, 10);
  EXPECT_EQ(this->parse(this->lua["small_cfg"]).parameter.knn_neighbor_count, 3);
  EXPECT_EQ(this->parse(this->lua["disabled_cfg"]).parameter.knn_neighbor_count, 0);
  EXPECT_THROW((void)this->parse(this->lua["negative_cfg"]), std::invalid_argument);
  EXPECT_THROW((void)this->parse(this->lua["overflow_cfg"]), std::invalid_argument);
  EXPECT_THROW((void)this->parse(this->lua["wrong_type_cfg"]), std::invalid_argument);
}

TEST_F(StraLcConfigTest, ImprovementNamesRoundTrip) {
  this->lua.script(R"(
    first_cfg = { improvement = "first" }
    best_cfg = { improvement = "best" }
  )");

  EXPECT_EQ(this->parse(this->lua["first_cfg"]).parameter.improvement, LsImprovement::First);
  EXPECT_EQ(this->parse(this->lua["best_cfg"]).parameter.improvement, LsImprovement::Best);
}

TEST_F(StraLcConfigTest, ExplicitConfigRoundTripsKeepsImprovement) {
  this->lua.script(R"(
    cfg = { epsilon = 1e-6, improvement = "first" }
  )");

  const LsStrategyConfig config = this->parse(this->lua["cfg"]);

  EXPECT_DOUBLE_EQ(config.parameter.epsilon, 1e-6);
  EXPECT_EQ(config.parameter.improvement, LsImprovement::First);
}

TEST_F(StraLcConfigTest, UnknownImprovementThrows) {
  this->lua.script(R"(
    cfg = { improvement = "greedy" }
  )");

  EXPECT_THROW((void)this->parse(this->lua["cfg"]), std::invalid_argument);
}

TEST_F(StraLcConfigTest, NonStringImprovementThrows) {
  this->lua.script(R"(
    cfg = { improvement = 1 }
  )");

  EXPECT_THROW((void)this->parse(this->lua["cfg"]), std::invalid_argument);
}

TEST_F(StraLcConfigTest, EmptyOperatorsKeepsEngineDefaultInstances) {
  this->lua.script(R"(
    cfg = { operators = {} }
  )");

  const LsStrategyConfig config = this->parse(this->lua["cfg"]);

  // 空数组在解析层保持为空, 由算法层展开为默认 7 个邻域实例
  EXPECT_TRUE(config.operators.empty());
  EXPECT_EQ(default_ls_operator_params().size(), 7U);
}

TEST_F(StraLcConfigTest, ExplicitConfigRoundTrips) {
  this->lua.script(R"(
    cfg = {
      epsilon = 1e-6,
      operators = {
        { code = "LsIntraRouteRelocate" },
        { code = "LsInterRouteRelocate",
          params = { min_chain_len = 2, max_chain_len = 3 } },
        { code = "LsIntraRouteExchange" },
        { code = "LsInterRouteExchange" },
        { code = "LsTwoOpt", params = { min_span_len = 3, max_span_len = 5 } },
        { code = "LsTwoOptStar", params = { min_tail_len = 4, max_tail_len = 6 } },
      },
    }
  )");

  const LsStrategyConfig config = this->parse(this->lua["cfg"]);

  EXPECT_DOUBLE_EQ(config.parameter.epsilon, 1e-6);

  ASSERT_EQ(config.operators.size(), 6U);

  ASSERT_TRUE(std::holds_alternative<LsSegmentRelocateParam>(config.operators[0]));
  const auto& relocate = std::get<LsSegmentRelocateParam>(config.operators[0]);
  EXPECT_EQ(relocate.min_chain_len, 1);
  EXPECT_EQ(relocate.max_chain_len, 1);
  EXPECT_EQ(relocate.route_scope, LsRouteScope::Intra);

  ASSERT_TRUE(std::holds_alternative<LsSegmentRelocateParam>(config.operators[1]));
  const auto& or_opt = std::get<LsSegmentRelocateParam>(config.operators[1]);
  EXPECT_EQ(or_opt.min_chain_len, 2);
  EXPECT_EQ(or_opt.max_chain_len, 3);
  EXPECT_EQ(or_opt.route_scope, LsRouteScope::Inter);

  ASSERT_TRUE(std::holds_alternative<LsExchangeParam>(config.operators[2]));
  EXPECT_EQ(std::get<LsExchangeParam>(config.operators[2]).route_scope, LsRouteScope::Intra);

  ASSERT_TRUE(std::holds_alternative<LsExchangeParam>(config.operators[3]));
  EXPECT_EQ(std::get<LsExchangeParam>(config.operators[3]).route_scope, LsRouteScope::Inter);

  ASSERT_TRUE(std::holds_alternative<LsTwoOptParam>(config.operators[4]));
  const auto& two_opt = std::get<LsTwoOptParam>(config.operators[4]);
  EXPECT_EQ(two_opt.min_span_len, 3);
  EXPECT_EQ(two_opt.max_span_len, 5);

  ASSERT_TRUE(std::holds_alternative<LsTwoOptStarParam>(config.operators[5]));
  const auto& two_opt_star = std::get<LsTwoOptStarParam>(config.operators[5]);
  EXPECT_EQ(two_opt_star.min_tail_len, 4);
  EXPECT_EQ(two_opt_star.max_tail_len, 6);
}

TEST_F(StraLcConfigTest, NonTableConfigThrows) {
  this->lua.script(R"(
    cfg = 1
  )");

  EXPECT_THROW((void)this->parse(this->lua["cfg"]), std::invalid_argument);
}

TEST_F(StraLcConfigTest, UnknownOperatorCodeThrows) {
  this->lua.script(R"(
    cfg = { operators = { { code = "LsNotExist" } } }
  )");

  try {
    const LsStrategyConfig config = this->parse(this->lua["cfg"]);
    FAIL() << "expected std::invalid_argument, got " << config.operators.size() << " operators";
  } catch (const std::invalid_argument& e) {
    EXPECT_NE(std::string(e.what()).find("LsNotExist"), std::string::npos);
  }
}

TEST_F(StraLcConfigTest, MissingOperatorCodeThrows) {
  this->lua.script(R"(
    cfg = { operators = { { params = {} } } }
  )");

  EXPECT_THROW((void)this->parse(this->lua["cfg"]), std::invalid_argument);
}

TEST_F(StraLcConfigTest, NonTableOperatorsThrows) {
  this->lua.script(R"(
    cfg = { operators = 1 }
  )");

  EXPECT_THROW((void)this->parse(this->lua["cfg"]), std::invalid_argument);
}

TEST_F(StraLcConfigTest, NonTableOperatorParamsThrows) {
  this->lua.script(R"(
    cfg = { operators = { { code = "LsInterRouteExchange", params = 1 } } }
  )");

  EXPECT_THROW((void)this->parse(this->lua["cfg"]), std::invalid_argument);
}

TEST_F(StraLcConfigTest, WrongFieldTypeThrows) {
  this->lua.script(R"(
    cfg = { epsilon = "tight" }
  )");
  EXPECT_THROW((void)this->parse(this->lua["cfg"]), std::invalid_argument);

  this->lua.script(R"(
    cfg2 = { operators = { { code = "LsIntraRouteRelocate", params = { min_chain_len = "two" } } } }
  )");
  EXPECT_THROW((void)this->parse(this->lua["cfg2"]), std::invalid_argument);
}
