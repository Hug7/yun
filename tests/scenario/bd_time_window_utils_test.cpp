/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#include "bdm_time_window_utils.h"

#include <gtest/gtest.h>

#include <memory>
#include <vector>

#include "bdm_time_window.h"
#include "bdm_time_window_plan.h"

namespace {

void ExpectTimeWindow(const TimeWindow* tw, long early, long late) {
  ASSERT_NE(tw, nullptr);
  EXPECT_EQ(tw->early, early);
  EXPECT_EQ(tw->late, late);
}

void ExpectPlan(TimeWindowPlan::UPtr& plan, long early_arr, long late_arr, long early_dest,
                long late_dest, long wait_time, long over_time) {
  ASSERT_NE(plan, nullptr);
  EXPECT_EQ(plan->early_arr, early_arr);
  EXPECT_EQ(plan->late_arr, late_arr);
  EXPECT_EQ(plan->early_dest, early_dest);
  EXPECT_EQ(plan->late_dest, late_dest);
  EXPECT_EQ(plan->wait_time, wait_time);
  EXPECT_EQ(plan->over_time, over_time);
}

}  // namespace

TEST(TimeWindowUntilsIntersection, ReturnsOverlap) {
  TimeWindow tw_a(100, 200);
  TimeWindow tw_b(150, 250);

  TimeWindow* result = TimeWindowUntils::intersection(&tw_a, &tw_b);
  ExpectTimeWindow(result, 150, 200);
  delete result;
}

TEST(TimeWindowUntilsIntersection, ReturnsNullWhenDisjoint) {
  TimeWindow tw_a(100, 200);
  TimeWindow tw_b(201, 300);

  TimeWindow* result = TimeWindowUntils::intersection(&tw_a, &tw_b);
  EXPECT_EQ(result, nullptr);
}

// ===== TimeWindowUntilsIntersectionTwsArr =====

TEST(TimeWindowUntilsIntersectionTwsArr, EmptyArrayReturnsEmpty) {
  std::vector<std::vector<TimeWindow*>> tws_arr;

  std::vector<TimeWindow*> result = TimeWindowUntils::intersection_tws_arr(tws_arr);
  EXPECT_TRUE(result.empty());
}

TEST(TimeWindowUntilsIntersectionTwsArr, SingleGroupReturnsItsWindows) {
  std::vector<TimeWindow*> tws = {new TimeWindow(100, 200), new TimeWindow(300, 400)};
  std::vector<std::vector<TimeWindow*>> tws_arr = {tws};

  std::vector<TimeWindow*> result = TimeWindowUntils::intersection_tws_arr(tws_arr);
  ASSERT_EQ(result.size(), 2);
  ExpectTimeWindow(result[0], 100, 200);
  ExpectTimeWindow(result[1], 300, 400);

  for (TimeWindow* tw : result) {
    delete tw;
  }
  for (TimeWindow* tw : tws) {
    delete tw;
  }
}

TEST(TimeWindowUntilsIntersectionTwsArr, IntersectsOverlappingSingleWindowPerGroup) {
  std::vector<TimeWindow*> tws_a = {new TimeWindow(0, 100)};
  std::vector<TimeWindow*> tws_b = {new TimeWindow(10, 20)};
  std::vector<std::vector<TimeWindow*>> tws_arr = {tws_a, tws_b};

  std::vector<TimeWindow*> result = TimeWindowUntils::intersection_tws_arr(tws_arr);
  ASSERT_EQ(result.size(), 1);
  ExpectTimeWindow(result[0], 10, 20);

  delete result[0];
  delete tws_a[0];
  delete tws_b[0];
}

TEST(TimeWindowUntilsIntersectionTwsArr, ReturnsEmptyWhenNoOverlap) {
  std::vector<TimeWindow*> tws_a = {new TimeWindow(0, 10)};
  std::vector<TimeWindow*> tws_b = {new TimeWindow(20, 30)};
  std::vector<std::vector<TimeWindow*>> tws_arr = {tws_a, tws_b};

  std::vector<TimeWindow*> result = TimeWindowUntils::intersection_tws_arr(tws_arr);
  EXPECT_TRUE(result.empty());

  delete tws_a[0];
  delete tws_b[0];
}

TEST(TimeWindowUntilsIntersectionTwsArr, IntersectsMultipleWindowsAcrossThreeGroups) {
  std::vector<TimeWindow*> tws_a = {new TimeWindow(0, 5), new TimeWindow(7, 10)};
  std::vector<TimeWindow*> tws_b = {new TimeWindow(0, 10)};
  std::vector<TimeWindow*> tws_c = {new TimeWindow(0, 6), new TimeWindow(7, 10)};
  std::vector<std::vector<TimeWindow*>> tws_arr = {tws_a, tws_b, tws_c};

  std::vector<TimeWindow*> result = TimeWindowUntils::intersection_tws_arr(tws_arr);
  ASSERT_EQ(result.size(), 2);
  ExpectTimeWindow(result[0], 0, 5);
  ExpectTimeWindow(result[1], 7, 10);

  for (TimeWindow* tw : result) {
    delete tw;
  }
  for (TimeWindow* tw : tws_a) {
    delete tw;
  }
  for (TimeWindow* tw : tws_b) {
    delete tw;
  }
  for (TimeWindow* tw : tws_c) {
    delete tw;
  }
}

TEST(TimeWindowUntilsIntersectionTwsArr, IntersectsLaterWindowsInEachGroup) {
  std::vector<TimeWindow*> tws_a = {new TimeWindow(0, 10), new TimeWindow(20, 30)};
  std::vector<TimeWindow*> tws_b = {new TimeWindow(5, 25)};
  std::vector<TimeWindow*> tws_c = {new TimeWindow(8, 22)};
  std::vector<std::vector<TimeWindow*>> tws_arr = {tws_a, tws_b, tws_c};

  std::vector<TimeWindow*> result = TimeWindowUntils::intersection_tws_arr(tws_arr);
  ASSERT_EQ(result.size(), 2);
  ExpectTimeWindow(result[0], 8, 10);
  ExpectTimeWindow(result[1], 20, 22);

  for (TimeWindow* tw : result) {
    delete tw;
  }
  for (TimeWindow* tw : tws_a) {
    delete tw;
  }
  for (TimeWindow* tw : tws_b) {
    delete tw;
  }
  for (TimeWindow* tw : tws_c) {
    delete tw;
  }
}

TEST(TimeWindowUntilsIntersectionTwsArr, EmptyInnerGroupReturnsEmpty) {
  std::vector<TimeWindow*> tws_a;
  std::vector<TimeWindow*> tws_b = {new TimeWindow(0, 10)};
  std::vector<std::vector<TimeWindow*>> tws_arr = {tws_a, tws_b};

  std::vector<TimeWindow*> result = TimeWindowUntils::intersection_tws_arr(tws_arr);
  EXPECT_TRUE(result.empty());

  delete tws_b[0];
}

// ===== TimeWindowUntilsMergeTimeWindows =====
TEST(TimeWindowUntilsMergeTimeWindows, MergesContinuousWindows) {
  std::vector<TimeWindow*> tws = {new TimeWindow(100, 200), new TimeWindow(200, 300),
                                  new TimeWindow(1000, 1100)};

  std::vector<TimeWindow*> result = TimeWindowUntils::merge_time_windows(tws);
  ASSERT_EQ(result.size(), 2);
  ExpectTimeWindow(result[0], 100, 300);
  ExpectTimeWindow(result[1], 1000, 1100);

  for (TimeWindow* tw : result) {
    delete tw;
  }
}

TEST(TimeWindowUntilsMergeTimeWindows, KeepsDisjointWindowsSeparate) {
  std::vector<TimeWindow*> tws = {new TimeWindow(100, 200), new TimeWindow(900, 1000)};

  std::vector<TimeWindow*> result = TimeWindowUntils::merge_time_windows(tws);
  ASSERT_EQ(result.size(), 2);
  ExpectTimeWindow(result[0], 100, 200);
  ExpectTimeWindow(result[1], 900, 1000);

  for (TimeWindow* tw : result) {
    delete tw;
  }
}

TEST(TimeWindowUntilsMergeTimeWindows, MergesWindowsWithinGapThreshold) {
  std::vector<TimeWindow*> tws = {new TimeWindow(100, 200), new TimeWindow(700, 800)};

  std::vector<TimeWindow*> result = TimeWindowUntils::merge_time_windows(tws);
  ASSERT_EQ(result.size(), 1);
  ExpectTimeWindow(result[0], 100, 800);

  delete result[0];
}

TEST(TimeWindowUntilsMergeTimeWindows, EmptyInputReturnsEmpty) {
  std::vector<TimeWindow*> tws;

  std::vector<TimeWindow*> result = TimeWindowUntils::merge_time_windows(tws);
  EXPECT_TRUE(result.empty());
}

// ===== TimeWindowInferForwardInferSS =====
TEST(TimeWindowInferForwardSS, OverlapComputesArrivalAndDeparture) {
  TimeWindowPlan::UPtr pre = std::make_unique<TimeWindowPlan>(0, 0, 100, 200, 0, 0);
  TimeWindow ori(120, 180);

  TimeWindowPlan::UPtr result = TimeWindowInfer::forward_s_s(pre, 0, 30, &ori);
  ExpectPlan(result, 120, 180, 150, 210, 0, 0);
}

TEST(TimeWindowInferForwardSS, WaitCaseComputesWaitTime) {
  TimeWindowPlan::UPtr pre = std::make_unique<TimeWindowPlan>(0, 0, 100, 110, 0, 0);
  TimeWindow ori(200, 300);

  TimeWindowPlan::UPtr result = TimeWindowInfer::forward_s_s(pre, 0, 30, &ori);
  ExpectPlan(result, 110, 110, 230, 230, 90, 0);
}

TEST(TimeWindowInferForwardSS, OverTimeCaseComputesOverTime) {
  TimeWindowPlan::UPtr pre = std::make_unique<TimeWindowPlan>(0, 0, 300, 400, 0, 0);
  TimeWindow ori(100, 200);

  TimeWindowPlan::UPtr result = TimeWindowInfer::forward_s_s(pre, 0, 30, &ori);
  ExpectPlan(result, 300, 300, 330, 330, 0, 100);
}

TEST(TimeWindowInferForwardSM, NormalWindowProducesOnePlan) {
  TimeWindowPlan::UPtr pre = std::make_unique<TimeWindowPlan>(0, 0, 100, 200, 0, 0);
  std::vector<TimeWindow*> ori_tws = {new TimeWindow(120, 180)};
  TimeWindowPlan::VecUPtr plans;

  TimeWindowInfer::forward_s_m(pre, 0, 30, ori_tws, plans);
  ASSERT_EQ(plans.size(), 1);
  ExpectPlan(plans[0], 120, 180, 150, 210, 0, 0);

  delete ori_tws[0];
}

TEST(TimeWindowInferForwardSM, MultipleNormalWindowsProduceMultiplePlans) {
  TimeWindowPlan::UPtr pre = std::make_unique<TimeWindowPlan>(0, 0, 100, 200, 0, 0);
  std::vector<TimeWindow*> ori_tws = {new TimeWindow(120, 150), new TimeWindow(160, 180)};
  TimeWindowPlan::VecUPtr plans;

  TimeWindowInfer::forward_s_m(pre, 0, 30, ori_tws, plans);
  ASSERT_EQ(plans.size(), 2);
  ExpectPlan(plans[0], 120, 150, 150, 180, 0, 0);
  ExpectPlan(plans[1], 160, 180, 190, 210, 0, 0);

  for (TimeWindow* tw : ori_tws) {
    delete tw;
  }
}

TEST(TimeWindowInferForwardSM, WaitWindowProducesWaitPlan) {
  TimeWindowPlan::UPtr pre = std::make_unique<TimeWindowPlan>(0, 0, 100, 110, 0, 0);
  std::vector<TimeWindow*> ori_tws = {new TimeWindow(200, 300)};
  TimeWindowPlan::VecUPtr plans;

  TimeWindowInfer::forward_s_m(pre, 0, 30, ori_tws, plans);
  ASSERT_EQ(plans.size(), 1);
  ExpectPlan(plans[0], 110, 110, 230, 230, 90, 0);

  delete ori_tws[0];
}

TEST(TimeWindowInferForwardSM, OverTimeWindowProducesOverTimePlan) {
  TimeWindowPlan::UPtr pre = std::make_unique<TimeWindowPlan>(0, 0, 300, 400, 0, 0);
  std::vector<TimeWindow*> ori_tws = {new TimeWindow(100, 200)};
  TimeWindowPlan::VecUPtr plans;

  TimeWindowInfer::forward_s_m(pre, 0, 30, ori_tws, plans);
  ASSERT_EQ(plans.size(), 1);
  ExpectPlan(plans[0], 300, 300, 330, 330, 0, 100);

  delete ori_tws[0];
}

TEST(TimeWindowInferForward, SinglePrePlanAndSingleWindow) {
  TimeWindowPlan::VecUPtr pre_tws;
  pre_tws.push_back(std::make_unique<TimeWindowPlan>(0, 0, 100, 200, 0, 0));
  std::vector<TimeWindow*> ori_tws = {new TimeWindow(120, 180)};

  TimeWindowPlan::VecUPtr plans = TimeWindowInfer::forward(pre_tws, 0, 30, ori_tws);
  ASSERT_EQ(plans.size(), 1);
  ExpectPlan(plans[0], 120, 180, 150, 210, 0, 0);

  delete ori_tws[0];
}

TEST(TimeWindowInferForward, MultiplePrePlansAccumulatePlans) {
  TimeWindowPlan::VecUPtr pre_tws;
  pre_tws.push_back(std::make_unique<TimeWindowPlan>(0, 0, 100, 120, 0, 0));
  pre_tws.push_back(std::make_unique<TimeWindowPlan>(0, 0, 110, 130, 0, 0));
  std::vector<TimeWindow*> ori_tws = {new TimeWindow(105, 125)};

  TimeWindowPlan::VecUPtr plans = TimeWindowInfer::forward(pre_tws, 0, 30, ori_tws);
  ASSERT_EQ(plans.size(), 2);
  ExpectPlan(plans[0], 105, 120, 135, 150, 0, 0);
  ExpectPlan(plans[1], 110, 125, 140, 155, 0, 0);

  for (TimeWindow* tw : ori_tws) {
    delete tw;
  }
}
