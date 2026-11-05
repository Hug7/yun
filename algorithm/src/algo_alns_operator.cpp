/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#include "algo_alns_operator.h"

#include <algorithm>
#include <format>
#include <stdexcept>
#include <utility>

#include "algo_alns_repair_greedy.h"
#include "algo_alns_repair_regret_k.h"
#include "algo_alns_ruin_random_removal_load.h"
#include "algo_alns_ruin_random_removal_order.h"

// ====== implement of AlnsOperatorStat ======
double AlnsOperatorStat::avg() const {
  return this->usage > 0 ? this->score_sum / static_cast<double>(this->usage) : 0.0;
}

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

void AlnsOperatorManager::init_weights() {
  if (this->ruin_operator_count <= 0) {
    throw std::invalid_argument("AlnsOperatorManager: `ruin_operator_params` must not be empty");
  }
  if (this->repair_operator_count <= 0) {
    throw std::invalid_argument("AlnsOperatorManager: `repair_operator_params` must not be empty");
  }
  this->ruin_operator_weights.assign(this->ruin_operator_count,
                                     this->parameter->operator_initial_weight);
  this->repair_operator_weights.assign(this->repair_operator_count,
                                       this->parameter->operator_initial_weight);
  this->operator_combination_stats.assign(
      this->ruin_operator_count, std::vector<AlnsOperatorStat>(this->repair_operator_count));
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
  // 分别构建 ruin 和 repair 的累积权重轮盘
  const auto accumulate = [](const std::vector<double>& weights) {
    std::vector<double> accumulation_weights;
    accumulation_weights.reserve(weights.size());
    double total_weight = 0;
    for (const double weight : weights) {
      total_weight += weight;
      accumulation_weights.push_back(total_weight);
    }
    return std::pair{std::move(accumulation_weights), total_weight};
  };
  // 转动一次轮盘, 累积数组升序, 二分查找第一个 >= wheel_val 的位置即差距最小的那个
  const auto draw = [this](const std::vector<double>& accumulation_weights,
                           const double total_weight) {
    const double wheel_val = this->select_operator_random->random_double(0, total_weight);
    return static_cast<int>(std::distance(
        accumulation_weights.begin(), std::ranges::lower_bound(accumulation_weights, wheel_val)));
  };

  const auto [ruin_accumulation_weights, ruin_total_weight] =
      accumulate(this->ruin_operator_weights);
  const auto [repair_accumulation_weights, repair_total_weight] =
      accumulate(this->repair_operator_weights);

  std::vector<AlnsOperatorCombination::UPtr> operator_combinations;
  operator_combinations.reserve(turned_count);
  for (int u = 0; u < turned_count; ++u) {
    // ruin 与 repair 独立抽取, 组合概率为两组边缘权重之积
    operator_combinations.emplace_back(std::make_unique<AlnsOperatorCombination>(
        draw(ruin_accumulation_weights, ruin_total_weight),
        draw(repair_accumulation_weights, repair_total_weight)));
  }

  return operator_combinations;
}

void AlnsOperatorManager::update_operator_weights(
    const std::vector<AlnsOperatorCombination::UPtr>& operator_combinations) {
  const int ruin_count = static_cast<int>(this->ruin_operator_weights.size());
  const int repair_count = static_cast<int>(this->repair_operator_weights.size());
  // 边缘样本池: 同一得分原样计入该 ruin 和该 repair
  std::vector<double> ruin_scores(ruin_count, 0.0);
  std::vector<double> repair_scores(repair_count, 0.0);
  std::vector<int> ruin_usages(ruin_count, 0);
  std::vector<int> repair_usages(repair_count, 0);

  for (const auto& operator_combination : operator_combinations) {
    const int ruin_ind = operator_combination->ruin_operator_ind;
    const int repair_ind = operator_combination->repair_operator_ind;
    const double score = operator_combination->operator_score;
    // 二维统计矩阵
    AlnsOperatorStat& stat = this->operator_combination_stats[ruin_ind][repair_ind];
    stat.score_sum += score;
    ++stat.usage;
    // 两组边缘样本池
    ruin_scores[ruin_ind] += score;
    ++ruin_usages[ruin_ind];
    repair_scores[repair_ind] += score;
    ++repair_usages[repair_ind];
  }

  const double learning_factor = this->parameter->operator_learning_factor;
  const double min_weight = this->parameter->operator_min_weight;
  for (int i = 0; i < ruin_count; ++i) {
    if (ruin_usages[i] > 0) {
      const double weight = (1 - learning_factor) * this->ruin_operator_weights[i] +
                            learning_factor * ruin_scores[i] / static_cast<double>(ruin_usages[i]);
      // 下限避免算子被永久淘汰
      this->ruin_operator_weights[i] = std::max(weight, min_weight);
    }
  }
  for (int j = 0; j < repair_count; ++j) {
    if (repair_usages[j] > 0) {
      const double weight =
          (1 - learning_factor) * this->repair_operator_weights[j] +
          learning_factor * repair_scores[j] / static_cast<double>(repair_usages[j]);
      this->repair_operator_weights[j] = std::max(weight, min_weight);
    }
  }
}

std::string AlnsOperatorManager::format_statistics() const {
  const auto format_avg = [](const int usage, const double score_sum) {
    return usage > 0 ? std::format("{:.3f}", score_sum / static_cast<double>(usage))
                     : std::string("-");
  };
  const int ruin_count = static_cast<int>(this->operator_combination_stats.size());
  const int repair_count =
      ruin_count > 0 ? static_cast<int>(this->operator_combination_stats.front().size()) : 0;

  std::string text = std::format("alns operator statistics: ruin_count={} repair_count={}",
                                 ruin_count, repair_count);
  text += "\ncombination of ruin and repair operators:";
  for (int u = 0; u < ruin_count; ++u) {
    const auto& row = this->operator_combination_stats[u];
    for (int m = 0; m < repair_count; ++m) {
      const auto& stat = row[m];
      text += std::format("\n  ruin[{}]={} repair[{}]={} usage={} score_sum={:.1f} avg={}", u,
                          this->ruin_operators[u]->code, m, this->repair_operators[m]->code,
                          stat.usage, stat.score_sum, format_avg(stat.usage, stat.score_sum));
    }
  }
  // ruin 的边缘统计（行求和）
  text += "\nmarginal ruin operators:";
  for (int u = 0; u < ruin_count; ++u) {
    const auto& row = this->operator_combination_stats[u];
    double ruin_score_sum = 0;
    int ruin_usage = 0;
    for (const auto& stat : row) {
      ruin_score_sum += stat.score_sum;
      ruin_usage += stat.usage;
    }
    text += std::format("\n  ruin[{}]={} weight={:.3f} usage={} score_sum={:.1f} avg={}", u,
                        this->ruin_operators[u]->code, this->ruin_operator_weights[u], ruin_usage,
                        ruin_score_sum, format_avg(ruin_usage, ruin_score_sum));
  }
  // repair 的边缘统计（列求和）
  text += "\nmarginal repair operators:";
  for (int m = 0; m < repair_count; ++m) {
    double score_sum = 0;
    int usage = 0;
    for (int i = 0; i < ruin_count; ++i) {
      score_sum += this->operator_combination_stats[i][m].score_sum;
      usage += this->operator_combination_stats[i][m].usage;
    }
    text += std::format("\n  repair[{}]={} weight={:.3f} usage={} score_sum={:.1f} avg={}",
                        m, this->repair_operators[m]->code, this->repair_operator_weights[m], usage,
                        score_sum, format_avg(usage, score_sum));
  }
  return text;
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
      if (alns_ruin_operator_code == AlnsRuinOperatorCode::RuinRandomRemovalLoad) {
        alns_operator_manager->add_ruin_operator(
            new AlnsRuinRandomRemovalLoad(workspace->context, params));
      } else if (alns_ruin_operator_code == AlnsRuinOperatorCode::RuinRandomRemovalOrder) {
        alns_operator_manager->add_ruin_operator(
            new AlnsRuinRandomRemovalOrder(workspace->context, params));
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
 * @param parallel_executor 并行执行器
 */
void create_alns_repair_operator(AlnsOperatorManager* alns_operator_manager, const sol::table& cfg,
                                 const AlnsParameter* alns_parameter, const Workspace* workspace,
                                 ParallelExecutor* parallel_executor) {
  if (cfg[AlnsParameterLuaField::REPAIR_OPERATOR_PARAMS].valid()) {
    const sol::table repair_params =
        cfg[AlnsParameterLuaField::REPAIR_OPERATOR_PARAMS].get<sol::table>();
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
            new AlnsRepairGreedy(workspace->context, parallel_executor, params));
      } else if (alns_repair_operator_code == AlnsRepairOperatorCode::RepairRegretK) {
        alns_operator_manager->add_repair_operator(
            new AlnsRepairRegretK(workspace->context, parallel_executor, params));
      } else {
        throw std::invalid_argument("alns repair operator code: " + code + " is not undefined!");
      }
    });
  }
  // TODO repair算子为空，使用默认算子
}
}  // namespace

std::unique_ptr<AlnsOperatorManager> create_alns_operator(const sol::table& cfg,
                                                          const AlnsParameter* alns_parameter,
                                                          const Workspace* workspace,
                                                          ParallelExecutor* parallel_executor) {
  auto alns_operator_manager = std::make_unique<AlnsOperatorManager>(alns_parameter);
  // 创建ruin算子
  create_alns_ruin_operator(alns_operator_manager.get(), cfg, alns_parameter, workspace);
  // 创建repair算子
  create_alns_repair_operator(alns_operator_manager.get(), cfg, alns_parameter, workspace,
                              parallel_executor);
  // 初始化权重与统计矩阵
  alns_operator_manager->init_weights();
  return alns_operator_manager;
}
