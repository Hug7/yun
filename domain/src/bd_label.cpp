/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#include "bd_label.h"

// ====== implement of Label ======
Label::~Label() {
  for (auto value : this->values) {
    delete value;
  }
  this->values.clear();
  this->value_map.clear();
}

LabelValue* Label::add_label_value(const std::string& value) {
  if (this->value_map.find(value) != this->value_map.end()) {
    return nullptr;
  } else {
    int ind = this->generate_index->next();
    LabelValue* label_value = new LabelValue(value, ind);
    this->values.push_back(label_value);
    this->value_map[value] = label_value;
    ++this->len;
    return label_value;
  }
}

LabelValue* Label::get_label_value(const std::string& value) {
  auto it = this->value_map.find(value);
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
  const int len = this->label_values.size();
  for (int u = 0; u < len; ++u) {
    for (auto& label_value : labelset_value->label_values[u]) {
      this->label_values[u][label_value.first] = label_value.second;
    }
  }
}

// ====== implement of LabelsetValueBitset ======
void LabelsetValueBitset::add_label_value(int label_ind, LabelValue* label_value) {
  this->bitsets[label_ind]->set(label_value->ind);
}

void LabelsetValueBitset::merge(LabelsetValueBitset::UPtr& labelset_value_bitset) {
  const int len = this->bitsets.size();
  for (int u = 0; u < len; ++u) {
    this->bitsets[u]->call_union(labelset_value_bitset->bitsets[u].get());
  }
}

// ====== implement of Labelset ======
Labelset::~Labelset() {
  this->labels.clear();
  this->label_ind_map.clear();
}

void Labelset::add_label(Label* label) {
  if (this->label_ind_map.count(label->code)) {
    throw std::runtime_error("Label " + label->code + " already exists in Labelset");
  } else {
    this->labels.push_back(label);
    this->label_ind_map[label->code] = this->len;
    ++this->len;
  }
}

int Labelset::get_label_ind(const std::string& code) const {
  auto it = this->label_ind_map.find(code);
  if (it == this->label_ind_map.end()) {
    return -1;
  } else {
    return it->second;
  }
}

LabelsetValue* Labelset::empty_labelset_value() const {
  LabelsetValue* labelset_value = new LabelsetValue(this->len);
  for (int u = 0; u < this->len; ++u) {
    labelset_value->label_values[u] = std::unordered_map<int, LabelValue*>();
  }
  return labelset_value;
}

LabelsetValueBitset::UPtr Labelset::empty_labelset_value_bitset() const {
  auto labelset_value_bitset = std::make_unique<LabelsetValueBitset>(this->len);
  for (int u = 0; u < this->len; ++u) {
    labelset_value_bitset->bitsets[u] = std::make_unique<LabelValueBitset>(this->labels[u]->len);
  }
  return labelset_value_bitset;
}

// ====== implement of LabelManager ======
LabelManager::~LabelManager() {
  for (auto label : this->labels) {
    delete label;
  }
  this->labels.clear();
  this->label_map.clear();
  delete this->location_labelset;
  delete this->vehicle_model_labelset;
  delete this->order_labelset;
}

Label* LabelManager::create_label(const std::string& code, const std::string& name) {
  if (this->label_map.find(code) != this->label_map.end()) {
    throw std::runtime_error("Label " + code + " already exists in LabelManager");
  } else {
    int ind = this->generate_index->next();
    Label* label = new Label(code, name, ind);
    this->labels.push_back(label);
    this->label_map[code] = label;
    return label;
  }
}

Label* LabelManager::get_label(const std::string& code) {
  auto iter = this->label_map.find(code);
  if (iter != this->label_map.end()) {
    return iter->second;
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
