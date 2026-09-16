/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#include "bdm_common.h"

// ====== implement of GenerateIndex ======
int GenerateIndex::next() {
  ++this->ind;
  return this->ind;
}

// ====== implement of Bitset ======
Bitset::Bitset(const UPtr& other) {
  this->w = other->w;
  this->n = other->n;
}

void Bitset::set(size_t u) { w[u >> 6] |= (1ULL << (u & 63)); }

void Bitset::clear(size_t u) { w[u >> 6] &= ~(1ULL << (u & 63)); }

void Bitset::clear_all() { std::fill(w.begin(), w.end(), 0); }

bool Bitset::test(size_t u) const { return w[u >> 6] & (1ULL << (u & 63)); }

void Bitset::set_all() {
  // 将所有完整字组设置为全1
  for (auto& word : w) {
    word = ~0ULL;  // 所有位设为1
  }
  // 处理最后一个字组中多余的位（如果存在）
  size_t remaining = n % 64;
  if (remaining != 0) {
    // 清零多余的位
    w[w.size() - 1] &= (1ULL << remaining) - 1;
    // 或者直接：w[last_index] = (1ULL << remaining) - 1;
  }
}

void Bitset::call_union(const Bitset* o) {
  for (size_t u = 0; u < w.size(); ++u) {
    w[u] |= o->w[u];
  }
}

void Bitset::call_intersection(const Bitset* o) {
  for (size_t u = 0; u < w.size(); ++u) {
    w[u] &= o->w[u];
  }
}

void Bitset::call_union(const Bitset::UPtr& o) {
  for (size_t u = 0; u < w.size(); ++u) {
    w[u] |= o->w[u];
  }
}

void Bitset::call_intersection(const Bitset::UPtr& o) {
  for (size_t u = 0; u < w.size(); ++u) {
    w[u] &= o->w[u];
  }
}

int Bitset::sum() const {
  int c = 0;
  for (const auto x : w) c += __builtin_popcountll(x);
  return c;
}
