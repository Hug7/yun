/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#include "algo_alns_operator.h"

#include <gtest/gtest.h>

#include <algorithm>
#include <initializer_list>
#include <memory>
#include <sol/sol.hpp>
#include <stdexcept>
#include <string>
#include <tuple>
#include <unordered_map>
#include <vector>

#include "algo_alns_parameter.h"

namespace {

constexpr double kInitialWeight = 100.0;
constexpr double kMinWeight = 0.005;

/**
 * @brief ruin 算子测试替身
 * @details 只用于权重与统计逻辑, 不执行任何移除
 */
class FakeRuin final : public AlnsRuinOperator {
 public:
  FakeRuin() : AlnsRuinOperator(AlnsRuinOperatorCode::RuinRandomRemovalLoad, nullptr) {}

  std::unordered_map<int, AlnsRuinLoadActivity::UPtr> execute(Solution* /*sol*/) override {
    return {};
  }

  [[nodiscard]] std::string format_parameters() const override { return ""; }
};

/**
 * @brief repair 算子测试替身
 */
class FakeRepair final : public AlnsRepairOperator {
 public:
  FakeRepair() : AlnsRepairOperator(AlnsRepairOperatorCode::RepairGreedy, nullptr, nullptr) {}

  void execute(RepairMatrix* /*repair_matrix*/) override {}

  [[nodiscard]] std::string format_parameters() const override { return ""; }
};

/**
 * @brief 测试夹具：持有参数与算子管理器
 */
class OperatorManagerFixture {
 public:
  explicit OperatorManagerFixture(const double learning_factor) {
    this->parameter.operator_learning_factor = learning_factor;
    this->parameter.operator_initial_weight = kInitialWeight;
    this->parameter.operator_min_weight = kMinWeight;
    this->manager = std::make_unique<AlnsOperatorManager>(&this->parameter);
  }

  void add_operators(const int ruin_count, const int repair_count) {
    for (int i = 0; i < ruin_count; ++i) {
      this->manager->add_ruin_operator(new FakeRuin());
    }
    for (int j = 0; j < repair_count; ++j) {
      this->manager->add_repair_operator(new FakeRepair());
    }
  }

  AlnsOperatorManager& operator_manager() { return *this->manager; }

 private:
  AlnsParameter parameter;
  std::unique_ptr<AlnsOperatorManager> manager;
};

/**
 * @brief 构造一组指定 (ruin_ind, repair_ind, score) 的组合样本
 */
std::vector<AlnsOperatorCombination::UPtr> make_combinations(
    const std::initializer_list<std::tuple<int, int, double>>& samples) {
  std::vector<AlnsOperatorCombination::UPtr> operator_combinations;
  operator_combinations.reserve(samples.size());
  for (const auto& [ruin_ind, repair_ind, score] : samples) {
    auto combination = std::make_unique<AlnsOperatorCombination>(ruin_ind, repair_ind);
    combination->operator_score = score;
    operator_combinations.push_back(std::move(combination));
  }
  return operator_combinations;
}

}  // namespace

// ====== 初始化 ======
TEST(AlnsOperatorManagerTest, InitWeightsFillsBothArraysAndStatsMatrix) {
  OperatorManagerFixture fixture(0.5);
  fixture.add_operators(2, 3);
  fixture.operator_manager().init_weights();

  const auto& manager = fixture.operator_manager();
  ASSERT_EQ(manager.ruin_operator_weights.size(), 2);
  ASSERT_EQ(manager.repair_operator_weights.size(), 3);
  ASSERT_EQ(manager.operator_combination_stats.size(), 2);
  ASSERT_EQ(manager.operator_combination_stats.front().size(), 3);
  for (const double weight : manager.ruin_operator_weights) {
    EXPECT_DOUBLE_EQ(weight, kInitialWeight);
  }
  for (const double weight : manager.repair_operator_weights) {
    EXPECT_DOUBLE_EQ(weight, kInitialWeight);
  }
  for (const auto& row : manager.operator_combination_stats) {
    for (const auto& stat : row) {
      EXPECT_EQ(stat.usage, 0);
      EXPECT_DOUBLE_EQ(stat.score_sum, 0.0);
      EXPECT_DOUBLE_EQ(stat.avg(), 0.0);
    }
  }
}

TEST(AlnsOperatorManagerTest, InitWeightsRejectsEmptyOperatorSets) {
  OperatorManagerFixture empty_fixture(0.5);
  EXPECT_THROW(empty_fixture.operator_manager().init_weights(), std::invalid_argument);

  OperatorManagerFixture no_repair_fixture(0.5);
  no_repair_fixture.add_operators(1, 0);
  EXPECT_THROW(no_repair_fixture.operator_manager().init_weights(), std::invalid_argument);

  OperatorManagerFixture no_ruin_fixture(0.5);
  no_ruin_fixture.add_operators(0, 1);
  EXPECT_THROW(no_ruin_fixture.operator_manager().init_weights(), std::invalid_argument);
}

// ====== 轮盘 ======
TEST(AlnsOperatorManagerTest, RouletteProducesIndicesInsideRange) {
  OperatorManagerFixture fixture(0.5);
  fixture.add_operators(2, 3);
  fixture.operator_manager().init_weights();

  const auto operator_combinations = fixture.operator_manager().roulette_wheel_operator(200);
  ASSERT_EQ(operator_combinations.size(), 200);
  for (const auto& operator_combination : operator_combinations) {
    EXPECT_GE(operator_combination->ruin_operator_ind, 0);
    EXPECT_LT(operator_combination->ruin_operator_ind, 2);
    EXPECT_GE(operator_combination->repair_operator_ind, 0);
    EXPECT_LT(operator_combination->repair_operator_ind, 3);
  }
}

// ====== 池化 EWMA ======
TEST(AlnsOperatorManagerTest, UpdateWeightsUsesPooledMarginalScores) {
  OperatorManagerFixture fixture(0.5);
  fixture.add_operators(2, 2);
  fixture.operator_manager().init_weights();

  // ruin 0: 5, 3 -> 均值 4; ruin 1: 1 -> 均值 1
  // repair 0: 5, 1 -> 均值 3; repair 1: 3 -> 均值 3
  fixture.operator_manager().update_operator_weights(make_combinations({
      {0, 0, 5},
      {0, 1, 3},
      {1, 0, 1},
  }));

  const auto& manager = fixture.operator_manager();
  EXPECT_DOUBLE_EQ(manager.ruin_operator_weights[0], 0.5 * kInitialWeight + 0.5 * 4);
  EXPECT_DOUBLE_EQ(manager.ruin_operator_weights[1], 0.5 * kInitialWeight + 0.5 * 1);
  EXPECT_DOUBLE_EQ(manager.repair_operator_weights[0], 0.5 * kInitialWeight + 0.5 * 3);
  EXPECT_DOUBLE_EQ(manager.repair_operator_weights[1], 0.5 * kInitialWeight + 0.5 * 3);
}

TEST(AlnsOperatorManagerTest, UpdateWeightsKeepsUnusedOperators) {
  OperatorManagerFixture fixture(0.5);
  fixture.add_operators(2, 2);
  fixture.operator_manager().init_weights();

  fixture.operator_manager().update_operator_weights(make_combinations({{0, 0, 5}}));

  const auto& manager = fixture.operator_manager();
  EXPECT_DOUBLE_EQ(manager.ruin_operator_weights[1], kInitialWeight);
  EXPECT_DOUBLE_EQ(manager.repair_operator_weights[1], kInitialWeight);
}

// ====== 二维统计矩阵 ======
TEST(AlnsOperatorManagerTest, StatsMatrixAccumulatesPerPair) {
  OperatorManagerFixture fixture(0.5);
  fixture.add_operators(2, 2);
  fixture.operator_manager().init_weights();

  fixture.operator_manager().update_operator_weights(make_combinations({
      {0, 0, 5},
      {0, 1, 3},
      {1, 0, 1},
  }));

  const auto& stats = fixture.operator_manager().operator_combination_stats;
  EXPECT_EQ(stats[0][0].usage, 1);
  EXPECT_DOUBLE_EQ(stats[0][0].score_sum, 5.0);
  EXPECT_DOUBLE_EQ(stats[0][0].avg(), 5.0);
  EXPECT_EQ(stats[0][1].usage, 1);
  EXPECT_DOUBLE_EQ(stats[0][1].score_sum, 3.0);
  EXPECT_EQ(stats[1][0].usage, 1);
  EXPECT_DOUBLE_EQ(stats[1][0].score_sum, 1.0);
  // 未命中的 cell 不除零
  EXPECT_EQ(stats[1][1].usage, 0);
  EXPECT_DOUBLE_EQ(stats[1][1].score_sum, 0.0);
  EXPECT_DOUBLE_EQ(stats[1][1].avg(), 0.0);
}

TEST(AlnsOperatorManagerTest, StatsMatrixAccumulatesAcrossIterations) {
  OperatorManagerFixture fixture(0.5);
  fixture.add_operators(1, 1);
  fixture.operator_manager().init_weights();

  fixture.operator_manager().update_operator_weights(make_combinations({{0, 0, 5}}));
  fixture.operator_manager().update_operator_weights(make_combinations({{0, 0, 3}}));

  const auto& stat = fixture.operator_manager().operator_combination_stats[0][0];
  EXPECT_EQ(stat.usage, 2);
  EXPECT_DOUBLE_EQ(stat.score_sum, 8.0);
  EXPECT_DOUBLE_EQ(stat.avg(), 4.0);
}

// ====== 权重下限 ======
TEST(AlnsOperatorManagerTest, UpdateWeightsNeverFallsBelowMinWeight) {
  OperatorManagerFixture fixture(0.9);
  fixture.add_operators(1, 1);
  fixture.operator_manager().init_weights();

  // 连续 0 分, EWMA 几何衰减, 但被下限截住
  for (int iter = 0; iter < 20; ++iter) {
    fixture.operator_manager().update_operator_weights(make_combinations({{0, 0, 0}}));
  }

  const auto& manager = fixture.operator_manager();
  EXPECT_DOUBLE_EQ(manager.ruin_operator_weights[0], kMinWeight);
  EXPECT_DOUBLE_EQ(manager.repair_operator_weights[0], kMinWeight);
}

// ====== 统计文本 ======
TEST(AlnsOperatorManagerTest, FormatStatisticsPrintsMatrixAndMarginals) {
  OperatorManagerFixture fixture(0.5);
  fixture.add_operators(2, 2);
  fixture.operator_manager().init_weights();
  fixture.operator_manager().update_operator_weights(make_combinations({
      {0, 0, 5},
      {1, 1, 1},
  }));

  const std::string text = fixture.operator_manager().format_statistics();
  EXPECT_NE(text.find("alns operator statistics: ruin_count=2 repair_count=2"), std::string::npos);
  EXPECT_NE(text.find("ruin[0]="), std::string::npos);
  EXPECT_NE(text.find("ruin[1]="), std::string::npos);
  EXPECT_NE(text.find("repair[0]="), std::string::npos);
  EXPECT_NE(text.find("avg=-"), std::string::npos);  // 未命中的 cell
  EXPECT_NE(text.find("\nmarginal ruin operators:"), std::string::npos);
  EXPECT_NE(text.find("\nmarginal repair operators:"), std::string::npos);
  EXPECT_NE(text.find("\n  repair[0]="), std::string::npos);
  EXPECT_NE(text.find("\n  repair[1]="), std::string::npos);
  // 换行数 = 1 组合区块头 + 4 单元格 + 1 ruin 边缘头 + 2 ruin + 1 repair 边缘头 + 2 repair
  EXPECT_EQ(static_cast<int>(std::count(text.begin(), text.end(), '\n')), 11);
}

// ====== 求解前打印算子 ======
TEST(AlnsOperatorManagerTest, FormatOperatorsPrintsCodesAndParameters) {
  sol::state lua;
  lua.open_libraries(sol::lib::base);
  lua.script(R"(
    cfg = {
      ruin_operator_params = {
        {
          code = "RuinRandomRemovalLoad",
          params = {
            select_load_min_rate = 0.2,
            select_load_max_rate = 0.5,
            select_load_random_seed = 7,
          },
        },
      },
      repair_operator_params = {
        { code = "RepairRegretK", params = { regret_k = 3 } },
      },
    }
  )");
  const sol::table cfg = lua["cfg"];
  AlnsParameter parameter;
  const auto manager = create_alns_operator(cfg, &parameter, nullptr, nullptr);

  const std::string text = manager->format_operators();
  EXPECT_NE(text.find("alns operators: ruin_count=1 repair_count=1"), std::string::npos);
  EXPECT_NE(text.find("ruin[0]=RuinRandomRemovalLoad params={select_load_random_seed=7, "
                      "select_load_min_rate=0.2, select_load_max_rate=0.5}"),
            std::string::npos);
  EXPECT_NE(text.find("repair[0]=RepairRegretK params={regret_k=3}"), std::string::npos);
}

// ====== 空算子兜底 ======
TEST(AlnsOperatorManagerTest, CreateAlnsOperatorUsesDefaultsWhenOperatorArraysMissing) {
  sol::state lua;
  lua.open_libraries(sol::lib::base);
  lua.script("cfg = {}");
  const sol::table cfg = lua["cfg"];
  AlnsParameter parameter;
  const auto manager = create_alns_operator(cfg, &parameter, nullptr, nullptr);

  ASSERT_EQ(manager->ruin_operator_count, 3);
  ASSERT_EQ(manager->repair_operator_count, 2);
  ASSERT_EQ(manager->ruin_operators.size(), 3);
  ASSERT_EQ(manager->repair_operators.size(), 2);
  EXPECT_EQ(manager->ruin_operators[0]->code, "RuinRandomRemovalLoad");
  EXPECT_EQ(manager->ruin_operators[1]->code, "RuinRandomRemovalOrder");
  EXPECT_EQ(manager->ruin_operators[2]->code, "RuinWorstRemovalOrder");
  EXPECT_EQ(manager->repair_operators[0]->code, "RepairGreedy");
  EXPECT_EQ(manager->repair_operators[1]->code, "RepairRegretK");
  // 权重与统计矩阵按默认算子数量初始化
  EXPECT_EQ(manager->ruin_operator_weights.size(), 3);
  EXPECT_EQ(manager->repair_operator_weights.size(), 2);
  EXPECT_EQ(manager->operator_combination_stats.size(), 3);
  EXPECT_EQ(manager->operator_combination_stats.front().size(), 2);
  // 默认算子使用算子自身的默认参数
  const std::string text = manager->format_operators();
  EXPECT_NE(text.find("ruin[0]=RuinRandomRemovalLoad params={select_load_random_seed=37, "
                      "select_load_min_rate=0.1, select_load_max_rate=0.4}"),
            std::string::npos);
  EXPECT_NE(text.find("ruin[1]=RuinRandomRemovalOrder params={select_order_random_seed=37, "
                      "select_order_min_rate=0.1, select_order_max_rate=0.4}"),
            std::string::npos);
  EXPECT_NE(text.find("ruin[2]=RuinWorstRemovalOrder params={select_order_random_seed=37, "
                      "select_order_min_rate=0.1, select_order_max_rate=0.3, "
                      "select_order_noise_exponent=3}"),
            std::string::npos);
  EXPECT_NE(text.find("repair[0]=RepairGreedy params={random_seed=37, top_rate=0.2}"),
            std::string::npos);
  EXPECT_NE(text.find("repair[1]=RepairRegretK params={regret_k=2}"), std::string::npos);
}

TEST(AlnsOperatorManagerTest, CreateAlnsOperatorFallsBackPerEmptyOperatorArray) {
  sol::state lua;
  lua.open_libraries(sol::lib::base);
  lua.script(R"(
    cfg = {
      ruin_operator_params = {},
      repair_operator_params = {
        { code = "RepairRegretK", params = { regret_k = 3 } },
      },
    }
  )");
  const sol::table cfg = lua["cfg"];
  AlnsParameter parameter;
  const auto manager = create_alns_operator(cfg, &parameter, nullptr, nullptr);

  // 只有空的 ruin 数组回退到默认算子, 用户自定义的 repair 保留
  EXPECT_EQ(manager->ruin_operator_count, 3);
  EXPECT_EQ(manager->ruin_operator_weights.size(), 3);
  EXPECT_EQ(manager->ruin_operators[2]->code, "RuinWorstRemovalOrder");
  ASSERT_EQ(manager->repair_operator_count, 1);
  EXPECT_EQ(manager->repair_operators[0]->code, "RepairRegretK");
  EXPECT_NE(manager->repair_operators[0]->format_parameters().find("regret_k=3"),
            std::string::npos);
}
