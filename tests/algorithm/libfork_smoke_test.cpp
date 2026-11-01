/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>

#include <atomic>

#include "algo_libfork.hpp"

namespace {

struct CountTask {
  auto operator()(auto, std::atomic<int>* counter) const -> lf::task<> {
    counter->fetch_add(1, std::memory_order_relaxed);
    co_return;
  }
};

}  // namespace

TEST(LibforkSmoke, RunsCoroutineOnPool) {
  lf::lazy_pool pool(2);
  std::atomic<int> counter{0};
  lf::sync_wait(pool, CountTask{}, &counter);
  EXPECT_EQ(counter.load(std::memory_order_relaxed), 1);
}
