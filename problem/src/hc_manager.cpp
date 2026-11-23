/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#include "hc_manager.h"

#include <format>
#include <magic_enum/magic_enum.hpp>
#include <stdexcept>
#include <string>
#include <unordered_set>
#include <vector>

#include "hc_constraint_code.h"
#include "hc_load_unload_policy.h"
#include "hc_vehicle.h"
#include "prob_policy.h"

// ====== implement of HardConstraintManager ======
HardConstraintManager::HardConstraintManager(const Scenario* scenario, const Parameter* parameter) {
  // 基础硬约束
  this->add_constr(new HcVehicleCapacity());
  this->add_constr(new HcAvailableVehicle());
  this->add_constr(new HcTimeWindow());

  // 参数配置硬约束
  if (parameter->load_unload_policy == LoadUnloadPolicyType::FILO) {
    this->add_constr(new HcLoadUnloadPolicyFILO());
  } else if (parameter->load_unload_policy == LoadUnloadPolicyType::FIFO) {
    this->add_constr(new HcLoadUnloadPolicyFIFO());
  }

  // 可配置硬约束
  for (const auto& hard_constr_item : parameter->hard_constr_items) {
    switch (hard_constr_item.code) {
      case HardConstrCode::MaxPickNodeCount:
        this->add_constr(HardConstrParser::max_pick_node_count(hard_constr_item));
        break;
      case HardConstrCode::MaxDropNodeCount:
        this->add_constr(HardConstrParser::max_drop_node_count(hard_constr_item));
        break;
      case HardConstrCode::LabelValueExclusive:
        this->add_constr(HardConstrParser::label_value_exclusive(scenario, hard_constr_item));
        break;
      case HardConstrCode::LabelValueCompatible:
        this->add_constr(HardConstrParser::label_value_compatible(scenario, hard_constr_item));
        break;
      case HardConstrCode::LabelValueMaxCount:
        this->add_constr(HardConstrParser::label_value_max_count(scenario, hard_constr_item));
        break;
      default:
        throw std::invalid_argument(std::format("unknown hard constraint code={}",
                                                magic_enum::enum_name(hard_constr_item.code)));
    }
  }
}

HardConstraintManager::~HardConstraintManager() {
  for (const auto& constraint : this->constrs) {
    delete constraint;
  }
}

void HardConstraintManager::add_constr(HardConstraint* constr) {
  this->constrs.push_back(constr);
  ++this->len;

  // sort the constraints in the order of their priority
  std::ranges::sort(this->constrs, [](const HardConstraint* a, const HardConstraint* b) {
    return a->priority > b->priority;
  });
}

void HardConstraintManager::eval_constrs(Load* load,
                                         const LoadConstrProfile::UPtr& constr_profile) const {
  for (const auto& constr : this->constrs) {
    auto hard_score = constr->eval(load);
    constr_profile->infeasible |= hard_score->is_infeasible();
    constr_profile->total_hard_penalty += hard_score->get_score();
    constr_profile->hard_constr_scores.push_back(std::move(hard_score));
  }
}

// ====== implement of HardConstrParser ======
HcMaxPickNodeCount* HardConstrParser::max_pick_node_count(const HardConstrItem& hard_constr_item) {
  const int max_pick_node_count = hard_constr_item.params.parse_int("max_pick_node_count");
  return new HcMaxPickNodeCount(max_pick_node_count);
}

HcMaxDropNodeCount* HardConstrParser::max_drop_node_count(const HardConstrItem& hard_constr_item) {
  const int max_drop_node_count = hard_constr_item.params.parse_int("max_drop_node_count");
  return new HcMaxDropNodeCount(max_drop_node_count);
}

namespace {

LabelValueEntity label_value_entity(const std::string& entity, const YamlNode& params) {
  if (entity == "Order") {
    return LabelValueEntity::Order;
  }
  if (entity == "Location") {
    return LabelValueEntity::Location;
  }
  params.fail("entity", std::format("不支持的标签实体={}，仅支持 Order 或 Location", entity));
  return LabelValueEntity::Order;
}

const Labelset* label_value_labelset(const Scenario* scenario, const LabelValueEntity entity,
                                     const YamlNode& params) {
  if (scenario->label_manager == nullptr) {
    params.fail("entity", "scenario 未初始化标签管理器");
  }
  return entity == LabelValueEntity::Order ? scenario->label_manager->order_labelset
                                           : scenario->label_manager->location_labelset;
}

LabelValueSide label_value_side(const LabelValueEntity entity, const YamlNode& params) {
  if (entity == LabelValueEntity::Order) {
    if (params.contains_key("side")) {
      params.fail("side", "entity 为 Order 时不支持 side 参数，side 仅用于 Location");
    }
    return LabelValueSide::Both;
  }
  if (!params.contains_key("side")) {
    return LabelValueSide::Both;
  }
  const std::string side = params.parse_string("side");
  if (side == "Pick") {
    return LabelValueSide::Pick;
  }
  if (side == "Drop") {
    return LabelValueSide::Drop;
  }
  if (side == "Both") {
    return LabelValueSide::Both;
  }
  params.fail("side", std::format("不支持的站点侧别={}，仅支持 Pick、Drop 或 Both", side));
  return LabelValueSide::Both;
}

std::vector<std::vector<std::string>> label_value_groups(const YamlNode& params) {
  try {
    return params.get_node("groups").as<std::vector<std::vector<std::string>>>();
  } catch (const YAML::Exception& e) {
    params.fail("groups", std::format("groups 必须是二维字符串数组: {}", e.what()));
  }
  return {};
}

struct ParsedLabelValueGroups {
  LabelValueScope scope;
  std::vector<std::vector<int>> groups;
};

LabelValueScope parse_label_value_scope(const Scenario* scenario,
                                        const HardConstrItem& hard_constr_item) {
  const std::string label_code = hard_constr_item.params.parse_string("label_code");
  const std::string entity_code = hard_constr_item.params.parse_string("entity");
  const LabelValueEntity entity = label_value_entity(entity_code, hard_constr_item.params);
  const LabelValueSide side = label_value_side(entity, hard_constr_item.params);
  const Labelset* labelset = label_value_labelset(scenario, entity, hard_constr_item.params);

  const int label_ind = labelset->get_label_ind(label_code);
  if (label_ind < 0) {
    hard_constr_item.params.fail(
        "label_code", std::format("标签编码 {} 不存在于 {} labelset", label_code, entity_code));
  }
  return {.labelset = labelset, .label_ind = label_ind, .entity = entity, .side = side};
}

ParsedLabelValueGroups parse_label_value_groups(const Scenario* scenario,
                                                const HardConstrItem& hard_constr_item,
                                                const bool allow_cross_group_duplicate) {
  const LabelValueScope scope = parse_label_value_scope(scenario, hard_constr_item);
  const Label* label = scope.labelset->labels[scope.label_ind];
  const std::string label_code = label->code;

  const auto raw_groups = label_value_groups(hard_constr_item.params);
  if (raw_groups.empty()) {
    hard_constr_item.params.fail("groups", "groups 不能为空");
  }

  std::vector<std::vector<int>> groups;
  groups.reserve(raw_groups.size());
  std::unordered_set<int> seen_values;
  for (size_t group_ind = 0; group_ind < raw_groups.size(); ++group_ind) {
    const auto& raw_group = raw_groups[group_ind];
    if (raw_group.empty()) {
      hard_constr_item.params.fail("groups", std::format("groups[{}] 不能为空", group_ind));
    }
    std::vector<int> group;
    group.reserve(raw_group.size());
    std::unordered_set<int> in_group;
    for (const auto& value_code : raw_group) {
      const LabelValue* value = label->get_label_value(value_code);
      if (value == nullptr) {
        hard_constr_item.params.fail(
            "groups", std::format("标签值 {} 不存在于标签 {}", value_code, label_code));
      }
      if (!in_group.insert(value->ind).second) {
        hard_constr_item.params.fail(
            "groups", std::format("groups[{}] 中标签值 {} 重复", group_ind, value_code));
      }
      if (!allow_cross_group_duplicate && !seen_values.insert(value->ind).second) {
        hard_constr_item.params.fail(
            "groups",
            std::format("标签值 {} 在多个组中重复（可拼模式的组必须两两不相交）", value_code));
      }
      group.push_back(value->ind);
    }
    groups.push_back(std::move(group));
  }

  return {.scope = scope, .groups = std::move(groups)};
}

}  // namespace

HcLabelValueExclusive* HardConstrParser::label_value_exclusive(
    const Scenario* scenario, const HardConstrItem& hard_constr_item) {
  auto parsed = parse_label_value_groups(scenario, hard_constr_item, true);
  return new HcLabelValueExclusive(parsed.scope, parsed.groups);
}

HcLabelValueCompatible* HardConstrParser::label_value_compatible(
    const Scenario* scenario, const HardConstrItem& hard_constr_item) {
  auto parsed = parse_label_value_groups(scenario, hard_constr_item, false);
  return new HcLabelValueCompatible(parsed.scope, parsed.groups);
}

HcLabelValueMaxCount* HardConstrParser::label_value_max_count(
    const Scenario* scenario, const HardConstrItem& hard_constr_item) {
  const LabelValueScope scope = parse_label_value_scope(scenario, hard_constr_item);
  const int max_count = hard_constr_item.params.parse_int("max_count");
  if (max_count < 0) {
    hard_constr_item.params.fail("max_count", "max_count 必须是非负整数");
  }
  return new HcLabelValueMaxCount(scope, max_count);
}
