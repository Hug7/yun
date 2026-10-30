/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#include "algo_alns_sa.h"

#include <gtest/gtest.h>

#include <cmath>
#include <stdexcept>

#include "algo_alns_parameter.h"

namespace {
constexpr double kScale = 4.0e6;  // 与算例 total_cost 同量级
}

// ====== 目标值尺度 ======
TEST(AlnsSaMath, ObjScaleUsesAbsOfInitCost) {
  EXPECT_DOUBLE_EQ(resolve_obj_scale(4.0e6), 4.0e6);
  EXPECT_DOUBLE_EQ(resolve_obj_scale(-4.0e6), 4.0e6);
  // 目标值为 0 或负值时不能直接做分母
  EXPECT_DOUBLE_EQ(resolve_obj_scale(0.0), 1.0);
  EXPECT_DOUBLE_EQ(resolve_obj_scale(-1e-12), 1.0);
  EXPECT_GT(resolve_obj_scale(0.0), 0.0);
}

// ====== metropolis 接受概率 ======
TEST(AlnsSaMath, NonPositiveDeltaAlwaysAccepted) {
  EXPECT_DOUBLE_EQ(metropolis_accept_probability(-1e5, kScale, 1e-2), 1.0);
  EXPECT_DOUBLE_EQ(metropolis_accept_probability(0.0, kScale, 1e-2), 1.0);
}

TEST(AlnsSaMath, MatchesRelativeTemperatureForm) {
  const double tau = 1e-2;
  // delta / (tau * f0) == 1 时 P == e^-1
  EXPECT_NEAR(metropolis_accept_probability(tau * kScale, kScale, tau), std::exp(-1.0), 1e-12);
  // 与绝对温度形式 exp(-delta / T_abs) 等价, T_abs = tau * f0
  const double delta = 5.0e4;
  EXPECT_NEAR(metropolis_accept_probability(delta, kScale, tau), std::exp(-delta / (tau * kScale)),
              1e-12);
}

TEST(AlnsSaMath, IsScaleInvariant) {
  // 算例规模放大 100 倍, delta 同比放大, 接受概率不变 => tau 可跨算例复用
  const double tau = 1e-2;
  const double p_small = metropolis_accept_probability(1.0e4, 1.0e6, tau);
  const double p_large = metropolis_accept_probability(1.0e6, 1.0e8, tau);
  EXPECT_NEAR(p_small, p_large, 1e-12);
}

TEST(AlnsSaMath, DecreasesWithDeltaAndIncreasesWithTemperature) {
  const double tau = 1e-2;
  const double p_tiny = metropolis_accept_probability(1.0e3, kScale, tau);
  const double p_big = metropolis_accept_probability(1.0e5, kScale, tau);
  EXPECT_GT(p_tiny, p_big);

  const double p_hot = metropolis_accept_probability(1.0e4, kScale, 1e-1);
  const double p_cold = metropolis_accept_probability(1.0e4, kScale, 1e-4);
  EXPECT_GT(p_hot, p_cold);
}

TEST(AlnsSaMath, DefaultTauAcceptsOnePercentWorseningWithExpMinusHalf) {
  AlnsParameter param;
  // tau_0 默认取 2e-2（调试得到的值），1% 恶化时 p = exp(-0.01 / 0.02) = e^-0.5。
  // 该默认值属于调参结论，变更时需要同步更新这个用例。
  EXPECT_DOUBLE_EQ(param.initial_temperature, 2.0e-2);
  const double p = metropolis_accept_probability(0.01 * kScale, kScale, param.initial_temperature);
  EXPECT_NEAR(p, std::exp(-0.5), 1e-12);
}

// ====== 冷却系数反解 ======
TEST(AlnsSaMath, AnnealingFactorLandsOnMinTemperature) {
  const double tau0 = 1e-2;
  const double tau_end = 2e-5;
  constexpr int iters = 200;
  const double alpha = resolve_annealing_factor(tau0, tau_end, iters);
  EXPECT_GT(alpha, 0.5);
  EXPECT_LT(alpha, 1.0);
  EXPECT_NEAR(tau0 * std::pow(alpha, iters), tau_end, 1e-15);
}

TEST(AlnsSaMath, AnnealingFactorDegenerateInputs) {
  EXPECT_DOUBLE_EQ(resolve_annealing_factor(1e-2, 2e-5, 0), 1.0);    // 不降温
  EXPECT_DOUBLE_EQ(resolve_annealing_factor(1e-2, 2e-5, -5), 1.0);   // 不降温
  EXPECT_DOUBLE_EQ(resolve_annealing_factor(1e-2, 1e-2, 200), 1.0);  // 首末相同
  EXPECT_DOUBLE_EQ(resolve_annealing_factor(0.0, 2e-5, 200), 1.0);
}

// ====== 参数默认值(防止回退成绝对温度) ======
TEST(AlnsParameterTest, DefaultTemperaturesAreRelative) {
  AlnsParameter param;
  EXPECT_GT(param.initial_temperature, 0.0);
  EXPECT_LT(param.initial_temperature, 1.0);  // 绝对温度 10000 会在这里失败
  EXPECT_GT(param.min_temperature, 0.0);
  EXPECT_LE(param.min_temperature, param.initial_temperature);
  EXPECT_LE(param.annealing_factor, 0.0);  // <= 0 表示自动推导
  EXPECT_GT(param.max_iter, 0);
}

// ====== 温度更新 ======
TEST(AlnsSaTest, CoolingIsMonotonicAndNeverBelowMin) {
  AlnsParameter param;
  param.annealing_factor = 0.97;  // 显式指定, 走固定冷却系数
  Solution sol;
  AlnsSimulatedAnnealing sa(&param, &sol, nullptr);

  double previous = sa.current_temperature();
  EXPECT_DOUBLE_EQ(previous, param.initial_temperature);
  for (int i = 0; i < 50; ++i) {
    sa.update_temperature();
    EXPECT_LT(sa.current_temperature(), previous);
    previous = sa.current_temperature();
  }
  for (int i = 0; i < 1000; ++i) {
    sa.update_temperature();
  }
  EXPECT_DOUBLE_EQ(sa.current_temperature(), param.min_temperature);

  // 目标值为 0 的空解: 尺度回落到 1.0, 不会除零
  EXPECT_DOUBLE_EQ(sa.objective_scale(), 1.0);
}

TEST(AlnsSaTest, AutoAnnealingFactorReachesMinTemperatureExactly) {
  AlnsParameter param;
  ASSERT_LE(param.annealing_factor, 0.0);
  Solution sol;
  AlnsSimulatedAnnealing sa(&param, &sol, nullptr);
  for (int i = 0; i < param.max_iter; ++i) {
    sa.update_temperature();
  }
  EXPECT_NEAR(sa.current_temperature(), param.min_temperature, 1e-15);
}

TEST(AlnsSaTest, RejectsInvalidTemperatureParameters) {
  AlnsParameter param;
  Solution sol;

  param.initial_temperature = 0.0;
  EXPECT_THROW(AlnsSimulatedAnnealing(&param, &sol, nullptr), std::invalid_argument);

  param.initial_temperature = 1e-2;
  param.min_temperature = 1.0;  // 高于初温
  EXPECT_THROW(AlnsSimulatedAnnealing(&param, &sol, nullptr), std::invalid_argument);
}

// ====== 接受判定 ======
TEST(AlnsSaTest, EqualSolutionIsAccepted) {
  AlnsParameter param;
  Solution sol;
  AlnsSimulatedAnnealing sa(&param, &sol, nullptr);
  EXPECT_EQ(sa.cal_accept_status(&sol, &sol, &sol), AlnsSaAcceptStatusCode::PROBABILITY_ACCEPT);
}
