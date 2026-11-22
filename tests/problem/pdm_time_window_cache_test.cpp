/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>

#include <unordered_map>
#include <vector>

#include "pdm_cache_time_window.h"
#include "pdm_time_window_pattern.h"

TEST(TimeWindowPatternRegistry, SameContentSharesId) {
  TimeWindowPatternRegistry registry;
  const int first = registry.intern({TimeWindow(0, 10), TimeWindow(20, 30)});
  const int second = registry.intern({TimeWindow(0, 10), TimeWindow(20, 30)});

  EXPECT_EQ(first, second);
  EXPECT_EQ(registry.size(), 1);
}

TEST(TimeWindowPatternRegistry, DifferentContentGetsDifferentId) {
  TimeWindowPatternRegistry registry;
  const int first = registry.intern({TimeWindow(0, 10)});
  const int second = registry.intern({TimeWindow(0, 11)});
  const int third = registry.intern({TimeWindow(0, 10), TimeWindow(20, 30)});

  EXPECT_NE(first, second);
  EXPECT_NE(first, third);
  EXPECT_EQ(registry.size(), 3);
}

TEST(TimeWindowPatternRegistry, EmptyWindowsIsAPattern) {
  TimeWindowPatternRegistry registry;
  const int empty = registry.intern({});
  const int non_empty = registry.intern({TimeWindow(0, 10)});

  EXPECT_NE(empty, non_empty);
  EXPECT_EQ(registry.size(), 2);
}

TEST(NodeTimeWindowKey, CanonicalizesOrderAndDuplicates) {
  // 节点上订单的遍历顺序由活动链决定, 且同一时间窗内容可能来自多个订单; 键必须只看集合
  std::vector<int> lhs{3, 1, 2, 1};
  tw_key_canonicalize(lhs);
  std::vector<int> rhs{2, 3, 1};
  tw_key_canonicalize(rhs);

  const NodeTimeWindowKey key(lhs);
  const NodeTimeWindowKeyView view(rhs);

  EXPECT_EQ(lhs, (std::vector<int>{1, 2, 3}));
  EXPECT_EQ(key.pattern_len, 3);
  EXPECT_EQ(key.hash_code, view.hash_code);
  EXPECT_TRUE(key == view);
  EXPECT_TRUE(view == key);
}

TEST(NodeTimeWindowKey, DifferentSetsDoNotCollide) {
  std::vector<int> lhs{1, 2};
  tw_key_canonicalize(lhs);
  std::vector<int> rhs{1, 3};
  tw_key_canonicalize(rhs);

  const NodeTimeWindowKey lhs_key(lhs);
  const NodeTimeWindowKey rhs_key(rhs);

  EXPECT_FALSE(lhs_key == rhs_key);
  EXPECT_NE(lhs_key.hash_code, rhs_key.hash_code);
}

TEST(NodeTimeWindowKey, EmptyKeyIsSharedByOtherNodes) {
  const NodeTimeWindowKey first;
  const NodeTimeWindowKey second;

  EXPECT_EQ(first.pattern_len, 0);
  EXPECT_TRUE(first == second);
}

TEST(NodeTimeWindowKey, HeterogeneousLookupFindsCanonicalEntry) {
  // 模拟冻结表: 存储键与查找视图来自不同的订单顺序, 命中同一条目
  std::unordered_map<NodeTimeWindowKey, int> cache;
  std::vector<int> stored{3, 1, 2};
  tw_key_canonicalize(stored);
  cache.emplace(NodeTimeWindowKey(stored), 42);

  std::vector<int> lookup{1, 2, 3, 3};
  tw_key_canonicalize(lookup);
  const auto it = cache.find(NodeTimeWindowKeyView(lookup));

  ASSERT_NE(it, cache.end());
  EXPECT_EQ(it->second, 42);
}

TEST(NodeTimeWindowCache, RecyclesPartiallyWhenFull) {
  NodeTimeWindowCache cache(nullptr);
  NodeTimeWindowStoreBuffer buffer;

  const int max_entries = CacheParameter::PICK_DROP_MAX_ENTRY_COUNT;
  const int target_entries = CacheParameter::PICK_DROP_RECYCLE_TARGET_COUNT;
  for (int i = 0; i < max_entries; ++i) {
    buffer.pick.emplace(NodeTimeWindowKey({i}), std::vector<TimeWindow>{TimeWindow(0, 1)});
  }
  cache.archive(&buffer);
  ASSERT_EQ(cache.pick.size(), static_cast<std::size_t>(max_entries));

  // 写缓冲已在上一次归档中清空, 这次归档只会触发按水位线的回收
  cache.archive(&buffer);
  EXPECT_EQ(cache.pick.size(), static_cast<std::size_t>(target_entries));
}
