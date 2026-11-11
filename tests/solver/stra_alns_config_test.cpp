/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#include "stra_alns_config.h"

#include <gtest/gtest.h>

#include <memory>
#include <sol/sol.hpp>
#include <stdexcept>
#include <string>
#include <variant>

#include "c_log.h"

namespace {

/**
 * @brief Lua→POD 解析测试夹具
 * @details logger 设为 off, 避免解析期间打印参数清单干扰测试输出
 */
class StraAlnsConfigTest : public ::testing::Test {
 protected:
  void SetUp() override {
    this->lua.open_libraries(sol::lib::base, sol::lib::table);
    this->logger = LogManager::create("stra_alns_config_test", "", "", "off");
  }

  sol::state lua;
  std::shared_ptr<spdlog::logger> logger;
};

}  // namespace

TEST_F(StraAlnsConfigTest, EmptyConfigKeepsGlobalDefaultsAndEmptyOperatorLists) {
  const sol::table cfg = this->lua.create_table();

  const AlnsStrategyConfig config = parse_alns_strategy_config(cfg, this->logger);

  ASSERT_NE(config.parameter, nullptr);
  const AlnsParameter default_parameter;
  EXPECT_EQ(config.parameter->max_iter, default_parameter.max_iter);
  EXPECT_EQ(config.parameter->default_random_seed, default_parameter.default_random_seed);
  EXPECT_TRUE(config.ruin_operator_params.empty());
  EXPECT_TRUE(config.repair_operator_params.empty());
}

TEST_F(StraAlnsConfigTest, RuinOperatorParamsAreConvertedToTypedPods) {
  this->lua.script(R"(
    cfg = {
      ruin_operator_params = {
        {
          code = "RuinRandomRemovalLoad",
          params = {
            select_load_random_seed = 7,
            select_load_min_rate = 0.2,
            select_load_max_rate = 0.5,
          },
        },
        {
          code = "RuinRandomRemovalOrder",
          params = { select_order_min_rate = 0.3 },
        },
        {
          code = "RuinWorstRemovalOrder",
          params = { select_order_noise_exponent = 2.5 },
        },
      },
    }
  )");
  const sol::table cfg = this->lua["cfg"];

  const AlnsStrategyConfig config = parse_alns_strategy_config(cfg, this->logger);

  ASSERT_EQ(config.ruin_operator_params.size(), 3U);

  const auto& load_param = std::get<AlnsRuinRandomRemovalLoadParam>(config.ruin_operator_params[0]);
  EXPECT_EQ(load_param.select_load_random_seed, 7);
  EXPECT_DOUBLE_EQ(load_param.select_load_min_rate, 0.2);
  EXPECT_DOUBLE_EQ(load_param.select_load_max_rate, 0.5);

  // 未显式配置的字段保持结构体默认值
  const AlnsRuinRandomRemovalOrderParam default_order_param{};
  const auto& order_param =
      std::get<AlnsRuinRandomRemovalOrderParam>(config.ruin_operator_params[1]);
  EXPECT_EQ(order_param.select_order_random_seed, default_order_param.select_order_random_seed);
  EXPECT_DOUBLE_EQ(order_param.select_order_min_rate, 0.3);
  EXPECT_DOUBLE_EQ(order_param.select_order_max_rate, default_order_param.select_order_max_rate);

  const auto& worst_param =
      std::get<AlnsRuinWorstRemovalOrderParam>(config.ruin_operator_params[2]);
  EXPECT_DOUBLE_EQ(worst_param.select_order_noise_exponent, 2.5);
}

TEST_F(StraAlnsConfigTest, RepairOperatorParamsAreConvertedToTypedPods) {
  this->lua.script(R"(
    cfg = {
      repair_operator_params = {
        { code = "RepairGreedy", params = { random_seed = 11, top_rate = 0.5 } },
        { code = "RepairRegretK", params = { regret_k = 4 } },
      },
    }
  )");
  const sol::table cfg = this->lua["cfg"];

  const AlnsStrategyConfig config = parse_alns_strategy_config(cfg, this->logger);

  ASSERT_EQ(config.repair_operator_params.size(), 2U);
  const auto& greedy_param = std::get<AlnsRepairGreedyParam>(config.repair_operator_params[0]);
  EXPECT_EQ(greedy_param.random_seed, 11);
  EXPECT_DOUBLE_EQ(greedy_param.top_rate, 0.5);

  const auto& regret_param = std::get<AlnsRepairRegretKParam>(config.repair_operator_params[1]);
  EXPECT_EQ(regret_param.regret_k, 4);
}

TEST_F(StraAlnsConfigTest, MissingOperatorParamsUsesStructDefaults) {
  this->lua.script(R"(
    cfg = {
      ruin_operator_params = { { code = "RuinRandomRemovalLoad" } },
      repair_operator_params = { { code = "RepairGreedy" } },
    }
  )");
  const sol::table cfg = this->lua["cfg"];

  const AlnsStrategyConfig config = parse_alns_strategy_config(cfg, this->logger);

  ASSERT_EQ(config.ruin_operator_params.size(), 1U);
  ASSERT_EQ(config.repair_operator_params.size(), 1U);

  const AlnsRuinRandomRemovalLoadParam default_load_param{};
  const auto& load_param = std::get<AlnsRuinRandomRemovalLoadParam>(config.ruin_operator_params[0]);
  EXPECT_EQ(load_param.select_load_random_seed, default_load_param.select_load_random_seed);
  EXPECT_DOUBLE_EQ(load_param.select_load_min_rate, default_load_param.select_load_min_rate);
  EXPECT_DOUBLE_EQ(load_param.select_load_max_rate, default_load_param.select_load_max_rate);

  const AlnsRepairGreedyParam default_greedy_param{};
  const auto& greedy_param = std::get<AlnsRepairGreedyParam>(config.repair_operator_params[0]);
  EXPECT_EQ(greedy_param.random_seed, default_greedy_param.random_seed);
  EXPECT_DOUBLE_EQ(greedy_param.top_rate, default_greedy_param.top_rate);
}

TEST_F(StraAlnsConfigTest, MissingOperatorCodeThrows) {
  this->lua.script(R"(
    cfg = { ruin_operator_params = { { params = {} } } }
  )");
  const sol::table cfg = this->lua["cfg"];

  try {
    const AlnsStrategyConfig config = parse_alns_strategy_config(cfg, this->logger);
    FAIL() << "expected std::runtime_error";
  } catch (const std::runtime_error& e) {
    EXPECT_NE(std::string(e.what()).find("missing 'code'"), std::string::npos);
  }
}

TEST_F(StraAlnsConfigTest, UnknownRuinOperatorCodeThrows) {
  this->lua.script(R"(
    cfg = { ruin_operator_params = { { code = "RuinNotExist" } } }
  )");
  const sol::table cfg = this->lua["cfg"];

  try {
    const AlnsStrategyConfig config = parse_alns_strategy_config(cfg, this->logger);
    FAIL() << "expected std::invalid_argument";
  } catch (const std::invalid_argument& e) {
    EXPECT_NE(std::string(e.what()).find("RuinNotExist"), std::string::npos);
  }
}

TEST_F(StraAlnsConfigTest, NonTableOperatorParamsThrows) {
  this->lua.script(R"(
    cfg = { repair_operator_params = { { code = "RepairGreedy", params = 1 } } }
  )");
  const sol::table cfg = this->lua["cfg"];

  EXPECT_THROW(parse_alns_strategy_config(cfg, this->logger), std::runtime_error);
}
