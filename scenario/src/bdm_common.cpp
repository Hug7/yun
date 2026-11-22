/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#include "bdm_common.h"

#include <algorithm>
#include <bit>
#include <cassert>

// ====== implement of GenerateIndex ======
int GenerateIndex::next() {
  ++this->ind;
  return this->ind;
}

// ====== implement of Bitset ======
Bitset::Bitset(const size_t n) : n_(n) {
  if (this->n_ > 64) {
    this->overflow_.assign((this->n_ + 63) / 64, 0);
  }
}

Bitset::Bitset(Bitset&& other) noexcept
    : inline_(other.inline_), overflow_(std::move(other.overflow_)), n_(other.n_) {
  other.inline_ = 0;
  other.overflow_.clear();
  other.n_ = 0;
}

Bitset& Bitset::operator=(Bitset&& other) noexcept {
  if (this != &other) {
    this->inline_ = other.inline_;
    this->overflow_ = std::move(other.overflow_);
    this->n_ = other.n_;
    other.inline_ = 0;
    other.overflow_.clear();
    other.n_ = 0;
  }
  return *this;
}

size_t Bitset::words_len() const { return (this->n_ + 63) / 64; }

uint64_t* Bitset::words() {
  return this->overflow_.empty() ? &this->inline_ : this->overflow_.data();
}

const uint64_t* Bitset::words() const {
  return this->overflow_.empty() ? &this->inline_ : this->overflow_.data();
}

void Bitset::set(size_t u) {
  assert(u < this->n_);
  this->words()[u >> 6] |= (1ULL << (u & 63));
}

void Bitset::clear(size_t u) {
  assert(u < this->n_);
  this->words()[u >> 6] &= ~(1ULL << (u & 63));
}

void Bitset::clear_all() {
  if (this->overflow_.empty()) {
    this->inline_ = 0;
  } else {
    std::fill(this->overflow_.begin(), this->overflow_.end(), 0);
  }
}

bool Bitset::test(size_t u) const {
  assert(u < this->n_);
  return this->words()[u >> 6] & (1ULL << (u & 63));
}

void Bitset::set_all() {
  if (this->n_ == 0) {
    return;
  }
  if (this->overflow_.empty()) {
    this->inline_ = ~0ULL;
    if (const size_t remaining = this->n_ % 64; remaining != 0) {
      this->inline_ &= (1ULL << remaining) - 1;
    }
    return;
  }
  std::fill(this->overflow_.begin(), this->overflow_.end(), ~0ULL);
  if (const size_t remaining = this->n_ % 64; remaining != 0) {
    this->overflow_.back() &= (1ULL << remaining) - 1;
  }
}

void Bitset::call_union(const Bitset& o) {
  assert(this->n_ == o.n_);
  uint64_t* dst = this->words();
  const uint64_t* src = o.words();
  const size_t len = this->words_len();
  for (size_t u = 0; u < len; ++u) {
    dst[u] |= src[u];
  }
}

void Bitset::call_intersection(const Bitset& o) {
  assert(this->n_ == o.n_);
  uint64_t* dst = this->words();
  const uint64_t* src = o.words();
  const size_t len = this->words_len();
  for (size_t u = 0; u < len; ++u) {
    dst[u] &= src[u];
  }
}

int Bitset::sum() const {
  int c = 0;
  const uint64_t* data = this->words();
  const size_t len = this->words_len();
  for (size_t u = 0; u < len; ++u) {
    c += std::popcount(data[u]);
  }
  return c;
}

Bitset BitsetUtils::call_intersection(const Bitset& a, const Bitset& b) {
  Bitset c = a;
  c.call_intersection(b);
  return c;
}

Bitset BitsetUtils::call_union(const Bitset& a, const Bitset& b) {
  Bitset c = a;
  c.call_union(b);
  return c;
}
