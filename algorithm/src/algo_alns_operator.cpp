/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#include "algo_alns_operator.h"

#include "algo_alns_repair_greedy.h"
#include "algo_alns_ruin_random.h"

// ====== implement of AlnsOperatorManager ======
AlnsOperatorManager::~AlnsOperatorManager() {
  delete this->select_operator_random;
  for (const auto& op : this->ruin_operators) {
    delete op;
  }
  for (const auto& op : this->repair_operators) {
    delete op;
  }
}

namespace {
/**
 * @brief 取算子的 params 表，字段缺失时返回空表，存在但不是表时报错
 */
sol::table get_operator_params(const sol::table& item) {
  const sol::object obj = item[AlnsParameterLuaField::OPERATOR_PARAMS];
  if (!obj.valid()) {
    return sol::table(item.lua_state(), sol::create);
  }
  if (!obj.is<sol::table>()) {
    throw std::runtime_error(std::string(AlnsParameterLuaField::OPERATOR_PARAMS) +
                             " must be a table");
  }
  return obj.as<sol::table>();
}
}  // namespace

void create_alns_ruin_operator(AlnsOperatorManager* alns_operator_manager, const sol::table& cfg,
                               const AlnsParameter* alns_parameter, const Workspace* workspace) {
  if (cfg[AlnsParameterLuaField::RUIN_OPERATOR_PARAMS].valid()) {
    sol::table ruin_params = cfg[AlnsParameterLuaField::RUIN_OPERATOR_PARAMS].get<sol::table>();
    ruin_params.for_each([&](sol::object key, sol::object value) {
      // key = 1, 2, 3... (数组下标)
      // value = 每个元素 table
      if (!value.is<sol::table>()) {
        return;  // 跳过非表元素
      }
      sol::table item = value.as<sol::table>();
      if (!item.valid()) {
        return;  // 防御：跳过非表元素
      }
      // 取 code（必填）
      std::string code = item[AlnsParameterLuaField::OPERATOR_CODE];
      if (code.empty()) {
        throw std::runtime_error(std::string(AlnsParameterLuaField::RUIN_OPERATOR_PARAMS) + "[" +
                                 std::to_string(key.as<int>()) + "]: missing '" +
                                 std::string(AlnsParameterLuaField::OPERATOR_CODE) + "'");
      }
      // 取 params（可选，缺省给空表）
      sol::table params = get_operator_params(item);
      // 转换ruin算子编码
      AlnsRuinOperatorCode alns_ruin_operator_code = parse_alns_ruin_operator_code(code);
      if (alns_ruin_operator_code == AlnsRuinOperatorCode::RuinRandomLoad) {
        alns_operator_manager->ruin_operators.push_back(new AlnsRuinRandomLoad(
            workspace->context, alns_parameter->default_random_seed, params));
      } else {
        throw std::invalid_argument("alns ruin operator code: " + code + " is not undefined!");
      }
    });
  }
  // TODO ruin算子为空，使用默认算子
}

void create_alns_repair_operator(AlnsOperatorManager* alns_operator_manager, const sol::table& cfg,
                                 const AlnsParameter* alns_parameter, const Workspace* workspace) {
  if (cfg[AlnsParameterLuaField::REPAIR_OPERATOR_PARAMS].valid()) {
    sol::table repair_params = cfg[AlnsParameterLuaField::REPAIR_OPERATOR_PARAMS].get<sol::table>();
    repair_params.for_each([&](sol::object key, sol::object value) {
      // key = 1, 2, 3... (数组下标)
      // value = 每个元素 table
      if (!value.is<sol::table>()) {
        return;  // 跳过非表元素
      }
      sol::table item = value.as<sol::table>();
      if (!item.valid()) {
        return;  // 防御：跳过非表元素
      }
      // 取 code（必填）
      std::string code = item[AlnsParameterLuaField::OPERATOR_CODE];
      if (code.empty()) {
        throw std::runtime_error(std::string(AlnsParameterLuaField::REPAIR_OPERATOR_PARAMS) + "[" +
                                 std::to_string(key.as<int>()) + "]: missing '" +
                                 std::string(AlnsParameterLuaField::OPERATOR_CODE) + "'");
      }
      // 取 params（可选，缺省给空表）
      sol::table params = get_operator_params(item);
      // 转换repair算子编码
      AlnsRepairOperatorCode alns_repair_operator_code = parse_alns_repair_operator_code(code);
      if (alns_repair_operator_code == AlnsRepairOperatorCode::RepairGreedy) {
        alns_operator_manager->repair_operators.push_back(new AlnsRepairGreedy(workspace->context));
      } else {
        throw std::invalid_argument("alns repair operator code: " + code + " is not undefined!");
      }
    });
  }
  // TODO ruin算子为空，使用默认算子
}

AlnsOperatorManager* create_alns_operator(const sol::table& cfg,
                                          const AlnsParameter* alns_parameter,
                                          const Workspace* workspace) {
  auto alns_operator_manager = new AlnsOperatorManager();
  // 创建ruin算子
  create_alns_ruin_operator(alns_operator_manager, cfg, alns_parameter, workspace);
  // 创建repair算子
  create_alns_repair_operator(alns_operator_manager, cfg, alns_parameter, workspace);
  // 初始化权重
  const size_t ruin_count = alns_operator_manager->ruin_operators.size();
  const size_t repair_count = alns_operator_manager->repair_operators.size();
  alns_operator_manager->operator_weights.assign(ruin_count,
                                                 std::vector<double>(repair_count, 100.0));
  return alns_operator_manager;
}
