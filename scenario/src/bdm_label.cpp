/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#include "bdm_label.h"

#include <algorithm>
#include <bit>
#include <cassert>
#include <stdexcept>

// ====== implement of Label ======
Label::~Label() {
  for (const auto& value : this->values) {
    delete value;
  }
  this->values.clear();
  this->value_map.clear();
}

LabelValue* Label::add_label_value(const std::string& value) {
  if (this->value_map.contains(value)) {
    return nullptr;
  } else {
    const int ind = this->generate_index->next();
    LabelValue* label_value = new LabelValue(value, ind);
    this->values.push_back(label_value);
    this->value_map[value] = label_value;
    ++this->len;
    return label_value;
  }
}

LabelValue* Label::get_label_value(const std::string& value) const {
  const auto it = this->value_map.find(value);
  if (it != this->value_map.end()) {
    return it->second;
  }

  return nullptr;
}

// ====== implement of LabelsetValue ======
LabelsetValue::~LabelsetValue() {
  for (auto& label_value_map : this->label_values) {
    label_value_map.clear();
  }
  this->label_values.clear();
}

void LabelsetValue::add_label_value(const int label_ind4labelset, LabelValue* label_value) {
  this->label_values[label_ind4labelset][label_value->ind] = label_value;
}

void LabelsetValue::merge(LabelsetValue* labelset_value) {
  const size_t len = this->label_values.size();
  for (size_t u = 0; u < len; ++u) {
    for (const auto& label_value : labelset_value->label_values[u]) {
      this->label_values[u][label_value.first] = label_value.second;
    }
  }
}

// ====== implement of Labelset ======
Labelset::~Labelset() {
  this->labels.clear();
  this->label_ind_map.clear();
}

void Labelset::add_label(Label* label) {
  if (this->label_ind_map.contains(label->code)) {
    throw std::runtime_error("Label " + label->code + " already exists in Labelset");
  } else {
    this->labels.push_back(label);
    this->label_ind_map[label->code] = this->len;
    ++this->len;
    // 标签集合变化后旧的 arena 布局不再有效
    this->label_word_offsets_.clear();
    this->words_len_ = 0;
    this->layout_built_ = false;
  }
}

bool Labelset::contains_label_code(const std::string& code) const {
  return this->label_ind_map.contains(code);
}

int Labelset::get_label_ind(const std::string& code) const {
  const auto it = this->label_ind_map.find(code);
  if (it == this->label_ind_map.end()) {
    return -1;
  } else {
    return it->second;
  }
}

Label* Labelset::get_label(const std::string& code) const {
  const int ind = this->get_label_ind(code);
  if (ind == -1) {
    return nullptr;
  }
  return this->labels[ind];
}

LabelsetValue* Labelset::empty_labelset_value() const {
  LabelsetValue* labelset_value = new LabelsetValue(this->len);
  for (int u = 0; u < this->len; ++u) {
    labelset_value->label_values[u] = std::unordered_map<int, LabelValue*>();
  }
  return labelset_value;
}

void Labelset::build_layout() const {
  if (this->layout_built_) {
    return;
  }
  this->label_word_offsets_.resize(static_cast<size_t>(this->len) + 1);
  size_t offset = 0;
  for (int u = 0; u < this->len; ++u) {
    this->label_word_offsets_[u] = static_cast<uint32_t>(offset);
    offset += (static_cast<size_t>(this->labels[u]->len) + 63) / 64;
  }
  this->label_word_offsets_[this->len] = static_cast<uint32_t>(offset);
  this->words_len_ = offset;
  this->layout_built_ = true;
}

size_t Labelset::labelset_words_len() const {
  this->build_layout();
  return this->words_len_;
}

size_t Labelset::label_word_offset(const int label_ind) const {
  this->build_layout();
  assert(label_ind >= 0 && label_ind <= this->len);
  return this->label_word_offsets_[label_ind];
}

LabelsetValueBitset::UPtr Labelset::empty_labelset_value_bitset() const {
  return std::make_unique<LabelsetValueBitset>(this);
}

// ====== implement of LabelsetValueBitset ======
LabelsetValueBitset::LabelsetValueBitset(const Labelset* labelset)
    : labelset_(labelset), words_(labelset == nullptr ? 0 : labelset->labelset_words_len(), 0) {
  assert(this->labelset_ != nullptr);
}

void LabelsetValueBitset::add_label_value(const int label_ind, LabelValue* label_value) {
  this->set(label_ind, label_value->ind);
}

bool LabelsetValueBitset::test(const int label_ind, const int value_ind) const {
  const size_t offset = this->labelset_->label_word_offset(label_ind);
  // 操作等价于: words_[offset + value_ind / 64] & (1ULL << (value_ind % 64))
  return this->words_[offset + (static_cast<size_t>(value_ind) >> 6)] & (1ULL << (value_ind & 63));
}

void LabelsetValueBitset::set(const int label_ind, const int value_ind) {
  const size_t offset = this->labelset_->label_word_offset(label_ind);
  // 操作等价于: words_[offset + value_ind / 64] & (1ULL << (value_ind % 64))
  this->words_[offset + (static_cast<size_t>(value_ind) >> 6)] |= (1ULL << (value_ind & 63));
}

int LabelsetValueBitset::sum(const int label_ind) const {
  const size_t begin = this->labelset_->label_word_offset(label_ind);
  const size_t end = this->labelset_->label_word_offset(label_ind + 1);
  int count = 0;
  for (size_t u = begin; u < end; ++u) {
    count += std::popcount(this->words_[u]);
  }
  return count;
}

void LabelsetValueBitset::merge(const LabelsetValueBitset& other) {
  assert(this->labelset_ == other.labelset_);
  assert(this->words_.size() == other.words_.size());
  for (size_t u = 0; u < this->words_.size(); ++u) {
    this->words_[u] |= other.words_[u];
  }
}

void LabelsetValueBitset::clear_bitsets() {
  std::ranges::fill(this->words_, 0);
}

// ====== implement of LabelManager ======
LabelManager::~LabelManager() {
  for (const auto& label : this->labels) {
    delete label;
  }
  this->labels.clear();
  this->label_map.clear();
  delete this->location_labelset;
  delete this->vehicle_model_labelset;
  delete this->order_labelset;
}

Label* LabelManager::create_label(const std::string& code, const std::string& name) {
  if (this->label_map.contains(code)) {
    throw std::runtime_error("Label " + code + " already exists in LabelManager");
  } else {
    const int ind = this->generate_index->next();
    Label* label = new Label(code, name, ind);
    this->labels.push_back(label);
    this->label_map[code] = label;
    return label;
  }
}

Label* LabelManager::get_label(const std::string& code) {
  const auto it = this->label_map.find(code);
  if (it != this->label_map.end()) {
    return it->second;
  } else {
    return nullptr;
  }
}

void LabelManager::labelset_add_label(const std::string& code, const std::string& labelset_item) {
  Label* label = this->get_label(code);
  if (label == nullptr) {
    throw std::runtime_error("Label " + code +
                             " is not exist in LabelManager.labelset_add_label(...)");
  }
  auto it = LabelSetSupportItem::item_map.find(labelset_item);
  if (it == LabelSetSupportItem::item_map.end()) {
    throw std::runtime_error("item " + labelset_item + " is not supported in LabelSetSupportItem");
  }

  if (it->second == LabelSetSupportItem::Location) {
    this->location_labelset->add_label(label);
  } else if (it->second == LabelSetSupportItem::VehicleModel) {
    this->vehicle_model_labelset->add_label(label);
  } else if (it->second == LabelSetSupportItem::Order) {
    this->order_labelset->add_label(label);
  } else if (it->second == LabelSetSupportItem::Carrier) {
    this->carrier_labelset->add_label(label);
  }
}
