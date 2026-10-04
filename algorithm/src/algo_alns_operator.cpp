/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#include "algo_alns_operator.h"

#include <algorithm>

#include "algo_alns_repair_greedy.h"
#include "algo_alns_ruin_random.h"

// ====== implement of AlnsOperatorCombination ======
void AlnsOperatorCombination::cal_operator_score(const AlnsSaAcceptStatusCode& status_code) {
  if (status_code == AlnsSaAcceptStatusCode::DOMINATE_BEST_SOLUTION) {
    this->operator_score = 5;
  } else if (status_code == AlnsSaAcceptStatusCode::DOMINATE_CURRENT_SOLUTION) {
    this->operator_score = 3;
  } else if (status_code == AlnsSaAcceptStatusCode::PROBABILITY_ACCEPT) {
    this->operator_score = 1;
  } else if (status_code == AlnsSaAcceptStatusCode::REJECT) {
    this->operator_score = 0;
  }
}

// ====== implement of AlnsOperatorManager ======
AlnsOperatorManager::~AlnsOperatorManager() {
  // 释放 select_operator_random
  delete this->select_operator_random;
  // 释放 ruin算子
  for (const auto& op : this->ruin_operators) {
    delete op;
  }
  // 释放 repair算子
  for (const auto& op : this->repair_operators) {
    delete op;
  }
}

void AlnsOperatorManager::add_ruin_operator(AlnsRuinOperator* ruin_operator) {
  this->ruin_operators.push_back(ruin_operator);
  ++this->ruin_operator_count;
}

void AlnsOperatorManager::add_repair_operator(AlnsRepairOperator* repair_operator) {
  this->repair_operators.push_back(repair_operator);
  ++this->repair_operator_count;
}

AlnsRuinOperator* AlnsOperatorManager::ruin_operator(
    const AlnsOperatorCombination::UPtr& operator_combination) const {
  return this->ruin_operators[operator_combination->ruin_operator_ind];
}

AlnsRepairOperator* AlnsOperatorManager::repair_operator(
    const AlnsOperatorCombination::UPtr& operator_combination) const {
  return this->repair_operators[operator_combination->repair_operator_ind];
}

std::vector<AlnsOperatorCombination::UPtr> AlnsOperatorManager::roulette_wheel_operator(
    const int turned_count) const {
  // 构建轮盘
  double total_weight = 0;                   // 总权重
  std::vector<double> accumulation_weights;  // 累积权重
  accumulation_weights.reserve(this->operator_combination_count);
  for (int u = 0; u < this->operator_combination_count; ++u) {
    total_weight += this->operator_combination_weights[u];
    accumulation_weights.push_back(total_weight);
  }
  // 转动轮盘
  std::vector<AlnsOperatorCombination::UPtr> operator_combinations;
  operator_combinations.reserve(turned_count);
  const int repair_count = static_cast<int>(this->repair_operators.size());
  for (int u = 0; u < turned_count; ++u) {
    const double wheel_val = this->select_operator_random->random_double(0, total_weight);
    // 二分查找累积权重中第一个 >= wheel_val 的位置，累积数组升序，故它即差距最小的那个
    const int combination_ind = static_cast<int>(std::distance(
        accumulation_weights.begin(), std::ranges::lower_bound(accumulation_weights, wheel_val)));
    // 展开索引还原为二维下标：[ruin_idx][repair_idx] -> ruin_ind * repair_count + repair_ind
    operator_combinations.emplace_back(std::make_unique<AlnsOperatorCombination>(
        combination_ind, combination_ind / repair_count, combination_ind % repair_count));
  }

  return operator_combinations;
}

void AlnsOperatorManager::update_operator_weights(
    const std::vector<AlnsOperatorCombination::UPtr>& operator_combinations) {
  std::vector<double> accumulation_scores;
  accumulation_scores.assign(this->operator_combination_count, 0.0);

  std::vector<int> operator_combination_usages;
  operator_combination_usages.assign(this->operator_combination_count, 0);

  for (const auto& operator_combination : operator_combinations) {
    accumulation_scores[operator_combination->operator_combination_ind] +=
        operator_combination->operator_score;
    ++operator_combination_usages[operator_combination->operator_combination_ind];
  }

  for (size_t u = 0; u < this->operator_combination_count; ++u) {
    if (operator_combination_usages[u] > 0) {
      this->operator_combination_weights[u] =
          (1 - this->parameter->operator_learning_factor) * this->operator_combination_weights[u];
      this->operator_combination_weights[u] += this->parameter->operator_learning_factor *
                                               accumulation_scores[u] /
                                               operator_combination_usages[u];
    }
  }
}

// ====== implement of create_alns_operator ======
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

/**
 * @brief 创建 alns 的 ruin 算子
 * @param alns_operator_manager alns算子管理器
 * @param cfg lua参数集合
 * @param alns_parameter alns算法参数
 * @param workspace 工作空间
 */
void create_alns_ruin_operator(AlnsOperatorManager* alns_operator_manager, const sol::table& cfg,
                               const AlnsParameter* alns_parameter, const Workspace* workspace) {
  if (cfg[AlnsParameterLuaField::RUIN_OPERATOR_PARAMS].valid()) {
    const sol::table ruin_params =
        cfg[AlnsParameterLuaField::RUIN_OPERATOR_PARAMS].get<sol::table>();
    ruin_params.for_each([&](const sol::object& key, const sol::object& value) {
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
      const std::string code = item[AlnsParameterLuaField::OPERATOR_CODE];
      if (code.empty()) {
        throw std::runtime_error(std::string(AlnsParameterLuaField::RUIN_OPERATOR_PARAMS) + "[" +
                                 std::to_string(key.as<int>()) + "]: missing '" +
                                 std::string(AlnsParameterLuaField::OPERATOR_CODE) + "'");
      }
      // 取 params（可选，缺省给空表）
      const sol::table params = get_operator_params(item);
      // 转换ruin算子编码
      const AlnsRuinOperatorCode alns_ruin_operator_code = convert_alns_ruin_operator_code(code);
      // 注册ruin算子
      if (alns_ruin_operator_code == AlnsRuinOperatorCode::RuinRandomLoad) {
        alns_operator_manager->add_ruin_operator(
            new AlnsRuinRandomLoad(workspace->context, params));
      } else {
        throw std::invalid_argument("alns ruin operator code: " + code + " is not undefined!");
      }
    });
  }
  // TODO ruin算子为空，使用默认算子
}

/**
 * @brief 创建 alns 的 repair 算子
 * @param alns_operator_manager alns算子管理器
 * @param cfg lua参数集合
 * @param alns_parameter alns算法参数
 * @param workspace 工作空间
 */
void create_alns_repair_operator(AlnsOperatorManager* alns_operator_manager, const sol::table& cfg,
                                 const AlnsParameter* alns_parameter, const Workspace* workspace,
                                 ThreadPool* thread_pool) {
  if (cfg[AlnsParameterLuaField::REPAIR_OPERATOR_PARAMS].valid()) {
    const sol::table repair_params = cfg[AlnsParameterLuaField::REPAIR_OPERATOR_PARAMS].get<sol::table>();
    repair_params.for_each([&](const sol::object& key, const sol::object& value) {
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
      const std::string code = item[AlnsParameterLuaField::OPERATOR_CODE];
      if (code.empty()) {
        throw std::runtime_error(std::string(AlnsParameterLuaField::REPAIR_OPERATOR_PARAMS) + "[" +
                                 std::to_string(key.as<int>()) + "]: missing '" +
                                 std::string(AlnsParameterLuaField::OPERATOR_CODE) + "'");
      }
      // 取 params（可选，缺省给空表）
      const sol::table params = get_operator_params(item);
      // 转换repair算子编码
      const AlnsRepairOperatorCode alns_repair_operator_code =
          parse_alns_repair_operator_code(code);
      // 注册repair算子
      if (alns_repair_operator_code == AlnsRepairOperatorCode::RepairGreedy) {
        alns_operator_manager->add_repair_operator(
            new AlnsRepairGreedy(workspace->context, thread_pool, params));
      } else {
        throw std::invalid_argument("alns repair operator code: " + code + " is not undefined!");
      }
    });
  }
  // TODO ruin算子为空，使用默认算子
}
}  // namespace

AlnsOperatorManager* create_alns_operator(const sol::table& cfg,
                                          const AlnsParameter* alns_parameter,
                                          const Workspace* workspace, ThreadPool* thread_pool) {
  const auto alns_operator_manager = new AlnsOperatorManager(alns_parameter);
  // 创建ruin算子
  create_alns_ruin_operator(alns_operator_manager, cfg, alns_parameter, workspace);
  // 创建repair算子
  create_alns_repair_operator(alns_operator_manager, cfg, alns_parameter, workspace, thread_pool);
  // 初始化权重
  alns_operator_manager->operator_combination_count =
      alns_operator_manager->ruin_operator_count * alns_operator_manager->repair_operator_count;
  alns_operator_manager->operator_combination_weights.assign(
      alns_operator_manager->operator_combination_count, alns_parameter->operator_initial_weight);
  return alns_operator_manager;
}
