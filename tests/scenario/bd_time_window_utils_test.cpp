/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>

#include <cstdint>
#include <optional>
#include <vector>

#include "bdm_time_window.h"
#include "bdm_time_window_plan.h"
#include "bdm_time_window_utils.h"

namespace {

void ExpectTimeWindow(const TimeWindow& tw, std::int64_t early, std::int64_t late) {
  EXPECT_EQ(tw.early, early);
  EXPECT_EQ(tw.late, late);
}

void ExpectPlan(const TimeWindowPlan& plan, std::int64_t early_arr, std::int64_t late_arr,
                std::int64_t early_dest, std::int64_t late_dest, std::int64_t wait_time,
                std::int64_t over_time) {
  EXPECT_EQ(plan.early_arr, early_arr);
  EXPECT_EQ(plan.late_arr, late_arr);
  EXPECT_EQ(plan.early_depart, early_dest);
  EXPECT_EQ(plan.late_depart, late_dest);
  EXPECT_EQ(plan.wait_time, wait_time);
  EXPECT_EQ(plan.over_time, over_time);
}

}  // namespace

TEST(TimeWindowUntilsIntersection, ReturnsOverlap) {
  const TimeWindow tw_a(100, 200);
  const TimeWindow tw_b(150, 250);

  const std::optional<TimeWindow> result = TimeWindowUntils::intersection(tw_a, tw_b);
  ASSERT_TRUE(result.has_value());
  ExpectTimeWindow(result.value(), 150, 200);
}

TEST(TimeWindowUntilsIntersection, ReturnsNullWhenDisjoint) {
  const TimeWindow tw_a(100, 200);
  const TimeWindow tw_b(201, 300);

  const std::optional<TimeWindow> result = TimeWindowUntils::intersection(tw_a, tw_b);
  EXPECT_FALSE(result.has_value());
}

// ===== TimeWindowUntilsIntersectionTwsArr =====

TEST(TimeWindowUntilsIntersectionTwsArr, EmptyArrayReturnsEmpty) {
  const std::vector<const std::vector<TimeWindow>*> tws_arr;

  const std::vector<TimeWindow> result = TimeWindowUntils::intersection_tws_arr(tws_arr);
  EXPECT_TRUE(result.empty());
}

TEST(TimeWindowUntilsIntersectionTwsArr, SingleGroupReturnsItsWindows) {
  const std::vector<TimeWindow> tws = {TimeWindow(100, 200), TimeWindow(300, 400)};
  const std::vector<const std::vector<TimeWindow>*> tws_arr = {&tws};

  const std::vector<TimeWindow> result = TimeWindowUntils::intersection_tws_arr(tws_arr);
  ASSERT_EQ(result.size(), 2);
  ExpectTimeWindow(result[0], 100, 200);
  ExpectTimeWindow(result[1], 300, 400);
}

TEST(TimeWindowUntilsIntersectionTwsArr, IntersectsOverlappingSingleWindowPerGroup) {
  const std::vector<TimeWindow> tws_a = {TimeWindow(0, 100)};
  const std::vector<TimeWindow> tws_b = {TimeWindow(10, 20)};
  const std::vector<const std::vector<TimeWindow>*> tws_arr = {&tws_a, &tws_b};

  const std::vector<TimeWindow> result = TimeWindowUntils::intersection_tws_arr(tws_arr);
  ASSERT_EQ(result.size(), 1);
  ExpectTimeWindow(result[0], 10, 20);
}

TEST(TimeWindowUntilsIntersectionTwsArr, ReturnsEmptyWhenNoOverlap) {
  const std::vector<TimeWindow> tws_a = {TimeWindow(0, 10)};
  const std::vector<TimeWindow> tws_b = {TimeWindow(20, 30)};
  const std::vector<const std::vector<TimeWindow>*> tws_arr = {&tws_a, &tws_b};

  const std::vector<TimeWindow> result = TimeWindowUntils::intersection_tws_arr(tws_arr);
  EXPECT_TRUE(result.empty());
}

TEST(TimeWindowUntilsIntersectionTwsArr, IntersectsMultipleWindowsAcrossThreeGroups) {
  const std::vector<TimeWindow> tws_a = {TimeWindow(0, 5), TimeWindow(7, 10)};
  const std::vector<TimeWindow> tws_b = {TimeWindow(0, 10)};
  const std::vector<TimeWindow> tws_c = {TimeWindow(0, 6), TimeWindow(7, 10)};
  const std::vector<const std::vector<TimeWindow>*> tws_arr = {&tws_a, &tws_b, &tws_c};

  const std::vector<TimeWindow> result = TimeWindowUntils::intersection_tws_arr(tws_arr);
  ASSERT_EQ(result.size(), 2);
  ExpectTimeWindow(result[0], 0, 5);
  ExpectTimeWindow(result[1], 7, 10);
}

TEST(TimeWindowUntilsIntersectionTwsArr, IntersectsLaterWindowsInEachGroup) {
  const std::vector<TimeWindow> tws_a = {TimeWindow(0, 10), TimeWindow(20, 30)};
  const std::vector<TimeWindow> tws_b = {TimeWindow(5, 25)};
  const std::vector<TimeWindow> tws_c = {TimeWindow(8, 22)};
  const std::vector<const std::vector<TimeWindow>*> tws_arr = {&tws_a, &tws_b, &tws_c};

  const std::vector<TimeWindow> result = TimeWindowUntils::intersection_tws_arr(tws_arr);
  ASSERT_EQ(result.size(), 2);
  ExpectTimeWindow(result[0], 8, 10);
  ExpectTimeWindow(result[1], 20, 22);
}

TEST(TimeWindowUntilsIntersectionTwsArr, EmptyInnerGroupReturnsEmpty) {
  const std::vector<TimeWindow> tws_a;
  const std::vector<TimeWindow> tws_b = {TimeWindow(0, 10)};
  const std::vector<const std::vector<TimeWindow>*> tws_arr = {&tws_a, &tws_b};

  const std::vector<TimeWindow> result = TimeWindowUntils::intersection_tws_arr(tws_arr);
  EXPECT_TRUE(result.empty());
}

// ===== TimeWindowUntilsMergeTimeWindows =====
TEST(TimeWindowUntilsMergeTimeWindows, MergesContinuousWindows) {
  const std::vector<TimeWindow> tws = {TimeWindow(100, 200), TimeWindow(200, 300),
                                       TimeWindow(1000, 1100)};

  const std::vector<TimeWindow> result = TimeWindowUntils::merge_time_windows(tws);
  ASSERT_EQ(result.size(), 2);
  ExpectTimeWindow(result[0], 100, 300);
  ExpectTimeWindow(result[1], 1000, 1100);
}

TEST(TimeWindowUntilsMergeTimeWindows, KeepsDisjointWindowsSeparate) {
  const std::vector<TimeWindow> tws = {TimeWindow(100, 200), TimeWindow(900, 1000)};

  const std::vector<TimeWindow> result = TimeWindowUntils::merge_time_windows(tws);
  ASSERT_EQ(result.size(), 2);
  ExpectTimeWindow(result[0], 100, 200);
  ExpectTimeWindow(result[1], 900, 1000);
}

TEST(TimeWindowUntilsMergeTimeWindows, MergesWindowsWithinGapThreshold) {
  const std::vector<TimeWindow> tws = {TimeWindow(100, 200), TimeWindow(700, 800)};

  const std::vector<TimeWindow> result = TimeWindowUntils::merge_time_windows(tws);
  ASSERT_EQ(result.size(), 1);
  ExpectTimeWindow(result[0], 100, 800);
}

TEST(TimeWindowUntilsMergeTimeWindows, EmptyInputReturnsEmpty) {
  const std::vector<TimeWindow> tws;

  const std::vector<TimeWindow> result = TimeWindowUntils::merge_time_windows(tws);
  EXPECT_TRUE(result.empty());
}

// ===== TimeWindowInferForwardInferSS =====
TEST(TimeWindowInferForwardSS, OverlapComputesArrivalAndDeparture) {
  const TimeWindowPlan pre(0, 0, 100, 200, 0, 0);
  const TimeWindow ori(120, 180);

  const TimeWindowPlan result = TimeWindowInfer::forward_s_s(pre, 0, 30, ori);
  ExpectPlan(result, 120, 180, 150, 210, 0, 0);
}

TEST(TimeWindowInferForwardSS, WaitCaseComputesWaitTime) {
  const TimeWindowPlan pre(0, 0, 100, 110, 0, 0);
  const TimeWindow ori(200, 300);

  const TimeWindowPlan result = TimeWindowInfer::forward_s_s(pre, 0, 30, ori);
  ExpectPlan(result, 110, 110, 230, 230, 90, 0);
}

TEST(TimeWindowInferForwardSS, OverTimeCaseComputesOverTime) {
  const TimeWindowPlan pre(0, 0, 300, 400, 0, 0);
  const TimeWindow ori(100, 200);

  const TimeWindowPlan result = TimeWindowInfer::forward_s_s(pre, 0, 30, ori);
  ExpectPlan(result, 300, 300, 330, 330, 0, 100);
}

TEST(TimeWindowInferForwardSM, NormalWindowProducesOnePlan) {
  const TimeWindowPlan pre(0, 0, 100, 200, 0, 0);
  const std::vector<TimeWindow> ori_tws = {TimeWindow(120, 180)};
  std::vector<TimeWindowPlan> plans;

  TimeWindowInfer::forward_s_m(pre, 0, 30, ori_tws, plans);
  ASSERT_EQ(plans.size(), 1);
  ExpectPlan(plans[0], 120, 180, 150, 210, 0, 0);
}

TEST(TimeWindowInferForwardSM, MultipleNormalWindowsProduceMultiplePlans) {
  const TimeWindowPlan pre(0, 0, 100, 200, 0, 0);
  const std::vector<TimeWindow> ori_tws = {TimeWindow(120, 150), TimeWindow(160, 180)};
  std::vector<TimeWindowPlan> plans;

  TimeWindowInfer::forward_s_m(pre, 0, 30, ori_tws, plans);
  ASSERT_EQ(plans.size(), 2);
  ExpectPlan(plans[0], 120, 150, 150, 180, 0, 0);
  ExpectPlan(plans[1], 160, 180, 190, 210, 0, 0);
}

TEST(TimeWindowInferForwardSM, WaitWindowProducesWaitPlan) {
  const TimeWindowPlan pre(0, 0, 100, 110, 0, 0);
  const std::vector<TimeWindow> ori_tws = {TimeWindow(200, 300)};
  std::vector<TimeWindowPlan> plans;

  TimeWindowInfer::forward_s_m(pre, 0, 30, ori_tws, plans);
  ASSERT_EQ(plans.size(), 1);
  ExpectPlan(plans[0], 110, 110, 230, 230, 90, 0);
}

TEST(TimeWindowInferForwardSM, OverTimeWindowProducesOverTimePlan) {
  const TimeWindowPlan pre(0, 0, 300, 400, 0, 0);
  const std::vector<TimeWindow> ori_tws = {TimeWindow(100, 200)};
  std::vector<TimeWindowPlan> plans;

  TimeWindowInfer::forward_s_m(pre, 0, 30, ori_tws, plans);
  ASSERT_EQ(plans.size(), 1);
  ExpectPlan(plans[0], 300, 300, 330, 330, 0, 100);
}

TEST(TimeWindowInferForward, SinglePrePlanAndSingleWindow) {
  std::vector<TimeWindowPlan> pre_tws;
  pre_tws.emplace_back(0, 0, 100, 200, 0, 0);
  const std::vector<TimeWindow> ori_tws = {TimeWindow(120, 180)};

  const std::vector<TimeWindowPlan> plans = TimeWindowInfer::forward(pre_tws, 0, 30, ori_tws);
  ASSERT_EQ(plans.size(), 1);
  ExpectPlan(plans[0], 120, 180, 150, 210, 0, 0);
}

TEST(TimeWindowInferForward, MultiplePrePlansAccumulatePlans) {
  std::vector<TimeWindowPlan> pre_tws;
  pre_tws.emplace_back(0, 0, 100, 120, 0, 0);
  pre_tws.emplace_back(0, 0, 110, 130, 0, 0);
  const std::vector<TimeWindow> ori_tws = {TimeWindow(105, 125)};

  const std::vector<TimeWindowPlan> plans = TimeWindowInfer::forward(pre_tws, 0, 30, ori_tws);
  ASSERT_EQ(plans.size(), 2);
  ExpectPlan(plans[0], 105, 120, 135, 150, 0, 0);
  ExpectPlan(plans[1], 110, 125, 140, 155, 0, 0);
}
