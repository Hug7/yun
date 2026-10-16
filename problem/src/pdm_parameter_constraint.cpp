/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#include "pdm_parameter_constraint.h"

#include <magic_enum/magic_enum.hpp>

namespace {
constexpr char KEY_CODE[] = "code";
constexpr char KEY_PARAMS[] = "params";
}  // namespace

std::vector<HardConstrItem> parse_hard_constr_items(const YamlNode& yaml_nodes) {
  std::vector<HardConstrItem> items;
  for (size_t u = 0; u < yaml_nodes.root_.size(); ++u) {
    const auto item_path = std::format("{}[{}]", yaml_nodes.path_, u);
    const auto entry = yaml_nodes.root_[u];
    if (!entry.IsMap()) {
      yaml_nodes.fail(item_path, "数据类型不符合预期`map`");
    }
    auto item_yaml_node = YamlNode(entry, item_path);
    auto item_code = item_yaml_node.parse_string(KEY_CODE);

    const auto constr_code = magic_enum::enum_cast<HardConstrCode>(item_code);
    if (!constr_code.has_value()) {
      throw std::runtime_error(std::format("{}: 未知硬约束编码={}", item_path, item_code));
    }

    if (!item_yaml_node.contains_key(KEY_PARAMS)) {
      items.emplace_back(*constr_code);
    } else {
      items.emplace_back(*constr_code, item_yaml_node.get_yaml_node(KEY_PARAMS));
    }
  }
  return items;
}

std::vector<SoftConstrItem> parse_soft_constr_items(const YamlNode& yaml_nodes) {
  std::vector<SoftConstrItem> items;
  for (size_t u = 0; u < yaml_nodes.root_.size(); ++u) {
    const auto item_path = std::format("{}[{}]", yaml_nodes.path_, u);
    const auto entry = yaml_nodes.root_[u];
    if (!entry.IsMap()) {
      yaml_nodes.fail(item_path, "数据类型不符合预期`map`");
    }
    auto item_yaml_node = YamlNode(entry, item_path);
    auto item_code = item_yaml_node.parse_string(KEY_CODE);

    const auto constr_code = magic_enum::enum_cast<SoftConstrCode>(item_code);
    if (!constr_code.has_value()) {
      throw std::runtime_error(std::format("{}: 未知软约束编码={}", item_path, item_code));
    }

    if (!item_yaml_node.contains_key(KEY_PARAMS)) {
      items.emplace_back(*constr_code);
    } else {
      items.emplace_back(*constr_code, item_yaml_node.get_yaml_node(KEY_PARAMS));
    }
  }
  return items;
}
