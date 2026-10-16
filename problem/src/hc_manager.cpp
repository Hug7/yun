/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#include "hc_manager.h"

#include <format>
#include <magic_enum/magic_enum.hpp>

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
      case HardConstrCode::OrderLabelValueMaxCount:
        this->add_constr(HardConstrParser::order_label_value_max_count(scenario, hard_constr_item));
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

HcOrderLabelValueMaxCount* HardConstrParser::order_label_value_max_count(
    const Scenario* scenario, const HardConstrItem& hard_constr_item) {

  // hard_constr_item.params.parse_list()
  // const auto node = hard_constr_item.params.require_map(KEY_MAX_COUNTS);
  // std::unordered_map<int, int> counts;
  // for (const auto& entry : node) {
  //   const auto label_code = entry.first.as<std::string>();
  //   const int label_ind = scenario->label_manager->order_labelset->get_label_ind(label_code);
  //   if (label_ind < 0) {
  //     fail(std::format("{}.{}.{}", params.path(), KEY_MAX_COUNTS, label_code), "未知 label code");
  //   }
  //   counts[label_ind] = entry.second.as<int>();
  // }
  // return counts;


  std::unordered_map<int, int> distinct_label_value_max_counts;
  return new HcOrderLabelValueMaxCount(distinct_label_value_max_counts);
}