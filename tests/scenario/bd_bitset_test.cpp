/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>

#include <cstddef>
#include <string>
#include <utility>

#include "bdm_common.h"
#include "bdm_label.h"

namespace {

void expect_all_set(Bitset& bitset, size_t n) {
  EXPECT_EQ(bitset.len(), n);
  EXPECT_EQ(bitset.sum(), static_cast<int>(n));
  for (size_t u = 0; u < n; ++u) {
    EXPECT_TRUE(bitset.test(u)) << "u=" << u << " n=" << n;
  }
}

void expect_all_clear(const Bitset& bitset, size_t n) {
  EXPECT_EQ(bitset.sum(), 0);
  for (size_t u = 0; u < n; ++u) {
    EXPECT_FALSE(bitset.test(u)) << "u=" << u << " n=" << n;
  }
}

}  // namespace

TEST(BitsetTest, Empty) {
  Bitset bitset(0);
  EXPECT_EQ(bitset.len(), 0);
  EXPECT_EQ(bitset.sum(), 0);
  bitset.clear_all();
  bitset.set_all();
  EXPECT_EQ(bitset.sum(), 0);
}

TEST(BitsetTest, SingleWordBoundaries) {
  for (const size_t n : {size_t{1}, size_t{63}, size_t{64}}) {
    Bitset bitset(n);
    expect_all_clear(bitset, n);
    bitset.set_all();
    expect_all_set(bitset, n);
    bitset.clear_all();
    expect_all_clear(bitset, n);
    bitset.set(0);
    bitset.set(n - 1);
    EXPECT_EQ(bitset.sum(), n == 1 ? 1 : 2);
    EXPECT_TRUE(bitset.test(0));
    EXPECT_TRUE(bitset.test(n - 1));
    bitset.clear(n - 1);
    EXPECT_EQ(bitset.sum(), n == 1 ? 0 : 1);
    EXPECT_FALSE(bitset.test(n - 1));
  }
}

TEST(BitsetTest, MultiWordBoundaries) {
  for (const size_t n : {size_t{65}, size_t{129}, size_t{1024}}) {
    Bitset bitset(n);
    expect_all_clear(bitset, n);
    bitset.set_all();
    expect_all_set(bitset, n);
    bitset.clear_all();
    expect_all_clear(bitset, n);
    bitset.set(0);
    bitset.set(64);
    bitset.set(n - 1);
    const int expected_sum = n == 65 ? 2 : 3;
    EXPECT_EQ(bitset.sum(), expected_sum);
    EXPECT_TRUE(bitset.test(0));
    EXPECT_TRUE(bitset.test(64));
    EXPECT_TRUE(bitset.test(n - 1));
    bitset.clear(64);
    EXPECT_EQ(bitset.sum(), expected_sum - 1);
    EXPECT_FALSE(bitset.test(64));
  }
}

TEST(BitsetTest, OverflowBitsStayZeroAfterSetAll) {
  Bitset bitset(65);
  bitset.set_all();
  EXPECT_EQ(bitset.sum(), 65);
  // 第 65 位不存在；只有第一个 word 的最高位和第二个 word 的最低位为 1
  EXPECT_TRUE(bitset.test(64));
}

TEST(BitsetTest, UnionAndIntersection) {
  Bitset a(129);
  Bitset b(129);
  a.set(0);
  a.set(64);
  a.set(128);
  b.set(64);
  b.set(65);

  Bitset intersection = BitsetUtils::call_intersection(a, b);
  EXPECT_EQ(intersection.len(), 129);
  EXPECT_EQ(intersection.sum(), 1);
  EXPECT_TRUE(intersection.test(64));
  EXPECT_FALSE(intersection.test(0));

  Bitset union_bitset = BitsetUtils::call_union(a, b);
  EXPECT_EQ(union_bitset.sum(), 4);
  EXPECT_TRUE(union_bitset.test(0));
  EXPECT_TRUE(union_bitset.test(64));
  EXPECT_TRUE(union_bitset.test(65));
  EXPECT_TRUE(union_bitset.test(128));

  // 原对象保持不变
  EXPECT_EQ(a.sum(), 3);
  EXPECT_EQ(b.sum(), 2);

  a.call_union(b);
  EXPECT_EQ(a.sum(), 4);
  a.call_intersection(b);
  EXPECT_EQ(a.sum(), 2);
  EXPECT_TRUE(a.test(64));
  EXPECT_TRUE(a.test(65));
}

TEST(BitsetTest, CopyAndMove) {
  Bitset original(129);
  original.set(0);
  original.set(64);
  original.set(128);

  Bitset copy = original;
  EXPECT_EQ(copy.len(), 129);
  EXPECT_EQ(copy.sum(), 3);
  copy.clear(0);
  EXPECT_EQ(original.sum(), 3);

  Bitset moved = std::move(original);
  EXPECT_EQ(moved.len(), 129);
  EXPECT_EQ(moved.sum(), 3);
  // 移后对象处于安全的空状态
  EXPECT_EQ(original.len(), 0);
  EXPECT_EQ(original.sum(), 0);

  Bitset assigned(200);
  assigned = moved;
  EXPECT_EQ(assigned.len(), 129);
  EXPECT_EQ(assigned.sum(), 3);
  EXPECT_EQ(moved.sum(), 3);

  Bitset move_assigned(7);
  move_assigned = std::move(assigned);
  EXPECT_EQ(move_assigned.len(), 129);
  EXPECT_EQ(move_assigned.sum(), 3);
  EXPECT_EQ(assigned.len(), 0);
  EXPECT_EQ(assigned.sum(), 0);
}

TEST(BitsetTest, MoveSmallBitset) {
  Bitset original(64);
  original.set(63);
  Bitset moved = std::move(original);
  EXPECT_EQ(moved.len(), 64);
  EXPECT_TRUE(moved.test(63));
  EXPECT_EQ(original.len(), 0);
  EXPECT_EQ(original.sum(), 0);
}

#ifndef NDEBUG
TEST(BitsetTest, LengthMismatchDies) {
  Bitset a(64);
  Bitset b(65);
  EXPECT_DEATH(a.call_union(b), "");
  EXPECT_DEATH(a.call_intersection(b), "");
}
#endif

TEST(LabelsetArenaTest, MultiWordLabelOffsets) {
  LabelManager manager;
  Label* wide_label = manager.create_label("wide", "wide");
  for (int u = 0; u < 70; ++u) {
    wide_label->add_label_value(std::to_string(u));
  }
  Label* narrow_label = manager.create_label("narrow", "narrow");
  for (int u = 0; u < 3; ++u) {
    narrow_label->add_label_value(std::to_string(u));
  }
  manager.labelset_add_label("wide", "Location");
  manager.labelset_add_label("narrow", "Location");

  const Labelset* labelset = manager.location_labelset;
  // wide 标签 70 个取值占 2 个 word，narrow 标签占 1 个 word
  EXPECT_EQ(labelset->labelset_words_len(), 3);
  EXPECT_EQ(labelset->label_word_offset(0), 0);
  EXPECT_EQ(labelset->label_word_offset(1), 2);
  EXPECT_EQ(labelset->label_word_offset(2), 3);

  auto bitset = labelset->empty_labelset_value_bitset();
  bitset->set(0, 0);
  bitset->set(0, 69);
  bitset->set(1, 2);
  EXPECT_EQ(bitset->sum(0), 2);
  EXPECT_EQ(bitset->sum(1), 1);
  EXPECT_TRUE(bitset->test(0, 0));
  EXPECT_TRUE(bitset->test(0, 69));
  EXPECT_FALSE(bitset->test(0, 64));
  EXPECT_TRUE(bitset->test(1, 2));

  LabelsetValueBitset copy = *bitset;
  copy.merge(*bitset);
  EXPECT_EQ(copy.sum(0), 2);
  EXPECT_EQ(copy.sum(1), 1);
  copy.clear_bitsets();
  EXPECT_EQ(copy.sum(0), 0);
  EXPECT_EQ(copy.sum(1), 0);
  EXPECT_EQ(bitset->sum(0), 2);
}
