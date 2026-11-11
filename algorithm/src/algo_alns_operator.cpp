/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#include "algo_alns_operator.h"

#include <algorithm>
#include <format>
#include <stdexcept>
#include <type_traits>
#include <utility>

#include "algo_alns_repair_greedy.h"
#include "algo_alns_repair_regret_k.h"
#include "algo_alns_ruin_random_removal_load.h"
#include "algo_alns_ruin_random_removal_order.h"
#include "algo_alns_ruin_worst_removal_order.h"

namespace {
/**
 * @brief Ropke & Pisinger (2006) 的算子奖励 σ 分数
 * @details σ1: 得到新的全局最优解; σ2: 优于当前解; σ3: 被 SA 接受但劣于当前解;
 *          σ4: 被拒绝。注意 σ3 > σ2 是 R&P 的刻意设计, 用于鼓励多样化。
 */
constexpr double kSigmaNewBest = 33.0;
constexpr double kSigmaBetterThanCurrent = 9.0;
constexpr double kSigmaAcceptedWorse = 13.0;
constexpr double kSigmaRejected = 0.0;
}  // namespace

// ====== implement of AlnsOperatorStat ======
double AlnsOperatorStat::avg() const {
  return this->usage > 0 ? this->score_sum / static_cast<double>(this->usage) : 0.0;
}

// ====== implement of AlnsOperatorCombination ======
void AlnsOperatorCombination::cal_operator_score(const AlnsSaAcceptStatusCode& status_code) {
  if (status_code == AlnsSaAcceptStatusCode::DOMINATE_BEST_SOLUTION) {
    this->operator_score = kSigmaNewBest;
  } else if (status_code == AlnsSaAcceptStatusCode::DOMINATE_CURRENT_SOLUTION) {
    this->operator_score = kSigmaBetterThanCurrent;
  } else if (status_code == AlnsSaAcceptStatusCode::PROBABILITY_ACCEPT) {
    this->operator_score = kSigmaAcceptedWorse;
  } else if (status_code == AlnsSaAcceptStatusCode::REJECT) {
    this->operator_score = kSigmaRejected;
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
  // segment 累计池
  this->segment_ruin_score_sums.assign(this->ruin_operator_count, 0.0);
  this->segment_repair_score_sums.assign(this->repair_operator_count, 0.0);
  this->segment_ruin_usages.assign(this->ruin_operator_count, 0);
  this->segment_repair_usages.assign(this->repair_operator_count, 0);
  this->segment_iter_count = 0;
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
  // 1) 累计: 二维观测统计整轮不清零; 边缘得分与使用次数按 segment 累计
  for (const auto& operator_combination : operator_combinations) {
    const int ruin_ind = operator_combination->ruin_operator_ind;
    const int repair_ind = operator_combination->repair_operator_ind;
    const double score = operator_combination->operator_score;
    // 二维统计矩阵
    AlnsOperatorStat& stat = this->operator_combination_stats[ruin_ind][repair_ind];
    stat.score_sum += score;
    ++stat.usage;
    // 当前 segment 的两组边缘样本池
    this->segment_ruin_score_sums[ruin_ind] += score;
    ++this->segment_ruin_usages[ruin_ind];
    this->segment_repair_score_sums[repair_ind] += score;
    ++this->segment_repair_usages[repair_ind];
  }

  // 2) 未到 segment 边界时只累计, 不更新权重
  ++this->segment_iter_count;
  if (this->segment_iter_count < this->parameter->operator_weight_update_interval_iter) {
    return;
  }

  // 3) 段末用池化均值按反应因子更新
  const double reaction_factor = this->parameter->operator_learning_factor;
  const double min_weight = this->parameter->operator_min_weight;
  const auto update_weights = [reaction_factor, min_weight](const std::vector<double>& score_sums,
                                                            const std::vector<int>& usages,
                                                            std::vector<double>& weights) {
    for (int i = 0; i < static_cast<int>(weights.size()); ++i) {
      if (usages[i] > 0) {
        const double mean_score = score_sums[i] / static_cast<double>(usages[i]);
        const double weight = (1.0 - reaction_factor) * weights[i] + reaction_factor * mean_score;
        // 下限避免算子被永久淘汰
        weights[i] = std::max(weight, min_weight);
      }
    }
  };
  update_weights(this->segment_ruin_score_sums, this->segment_ruin_usages,
                 this->ruin_operator_weights);
  update_weights(this->segment_repair_score_sums, this->segment_repair_usages,
                 this->repair_operator_weights);

  // 4) 重置 segment 累计池, 开始下一个 segment
  std::fill(this->segment_ruin_score_sums.begin(), this->segment_ruin_score_sums.end(), 0.0);
  std::fill(this->segment_repair_score_sums.begin(), this->segment_repair_score_sums.end(), 0.0);
  std::fill(this->segment_ruin_usages.begin(), this->segment_ruin_usages.end(), 0);
  std::fill(this->segment_repair_usages.begin(), this->segment_repair_usages.end(), 0);
  this->segment_iter_count = 0;
}

std::string AlnsOperatorManager::format_operators() const {
  std::string text = std::format("alns operators: ruin_count={} repair_count={}",
                                 this->ruin_operator_count, this->repair_operator_count);
  text += "\nruin operators:";
  for (int u = 0; u < this->ruin_operator_count; ++u) {
    const auto* ruin_operator = this->ruin_operators[u];
    text += std::format("\n  ruin[{}]={} params={{{}}}", u, ruin_operator->code,
                        ruin_operator->format_parameters());
  }
  text += "\nrepair operators:";
  for (int m = 0; m < this->repair_operator_count; ++m) {
    const auto* repair_operator = this->repair_operators[m];
    text += std::format("\n  repair[{}]={} params={{{}}}", m, repair_operator->code,
                        repair_operator->format_parameters());
  }
  return text;
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
    text += std::format("\n  repair[{}]={} weight={:.3f} usage={} score_sum={:.1f} avg={}", m,
                        this->repair_operators[m]->code, this->repair_operator_weights[m], usage,
                        score_sum, format_avg(usage, score_sum));
  }
  return text;
}

// ====== implement of create_alns_operator ======
namespace {
/**
 * @brief 默认的 ruin 算子参数
 * @details 元素顺序即默认注册顺序, 保证求解前打印与轮盘下标稳定
 */
const std::vector<AlnsRuinOperatorParam>& default_ruin_operator_params() {
  static const std::vector<AlnsRuinOperatorParam> operator_params{
      AlnsRuinRandomRemovalLoadParam{},
      AlnsRuinRandomRemovalOrderParam{},
      AlnsRuinWorstRemovalOrderParam{},
  };
  return operator_params;
}

/**
 * @brief 默认的 repair 算子参数
 * @details 元素顺序即默认注册顺序, 保证求解前打印与轮盘下标稳定
 */
const std::vector<AlnsRepairOperatorParam>& default_repair_operator_params() {
  static const std::vector<AlnsRepairOperatorParam> operator_params{
      AlnsRepairGreedyParam{},
      AlnsRepairRegretKParam{},
  };
  return operator_params;
}

/**
 * @brief 按结构化参数创建并注册 ruin 算子
 * @param alns_operator_manager alns算子管理器
 * @param operator_param ruin算子参数
 * @param context solver上下文
 */
void create_ruin_operator(AlnsOperatorManager* alns_operator_manager,
                          const AlnsRuinOperatorParam& operator_param,
                          const SolverContext* context) {
  std::visit(
      [alns_operator_manager, context]<typename Param>(const Param& param) {
        if constexpr (std::is_same_v<Param, AlnsRuinRandomRemovalLoadParam>) {
          alns_operator_manager->add_ruin_operator(new AlnsRuinRandomRemovalLoad(context, param));
        } else if constexpr (std::is_same_v<Param, AlnsRuinRandomRemovalOrderParam>) {
          alns_operator_manager->add_ruin_operator(new AlnsRuinRandomRemovalOrder(context, param));
        } else {
          static_assert(std::is_same_v<Param, AlnsRuinWorstRemovalOrderParam>,
                        "unhandled AlnsRuinOperatorParam alternative");
          alns_operator_manager->add_ruin_operator(new AlnsRuinWorstRemovalOrder(context, param));
        }
      },
      operator_param);
}

/**
 * @brief 按结构化参数创建并注册 repair 算子
 * @param alns_operator_manager alns算子管理器
 * @param operator_param repair算子参数
 * @param context solver上下文
 * @param parallel_executor 并行执行器
 */
void create_repair_operator(AlnsOperatorManager* alns_operator_manager,
                            const AlnsRepairOperatorParam& operator_param,
                            const SolverContext* context, ParallelExecutor* parallel_executor) {
  std::visit(
      [alns_operator_manager, context, parallel_executor]<typename Param>(const Param& param) {
        if constexpr (std::is_same_v<Param, AlnsRepairGreedyParam>) {
          alns_operator_manager->add_repair_operator(
              new AlnsRepairGreedy(context, parallel_executor, param));
        } else {
          static_assert(std::is_same_v<Param, AlnsRepairRegretKParam>,
                        "unhandled AlnsRepairOperatorParam alternative");
          alns_operator_manager->add_repair_operator(
              new AlnsRepairRegretK(context, parallel_executor, param));
        }
      },
      operator_param);
}
}  // namespace

std::unique_ptr<AlnsOperatorManager> create_alns_operator(
    const std::vector<AlnsRuinOperatorParam>& ruin_operator_params,
    const std::vector<AlnsRepairOperatorParam>& repair_operator_params,
    const AlnsParameter* alns_parameter, const SolverContext* context,
    ParallelExecutor* parallel_executor) {
  auto alns_operator_manager = std::make_unique<AlnsOperatorManager>(alns_parameter);
  // 创建ruin算子, 为空时使用默认算子
  const auto& ruin_params =
      ruin_operator_params.empty() ? default_ruin_operator_params() : ruin_operator_params;
  for (const auto& operator_param : ruin_params) {
    create_ruin_operator(alns_operator_manager.get(), operator_param, context);
  }
  // 创建repair算子, 为空时使用默认算子
  const auto& repair_params =
      repair_operator_params.empty() ? default_repair_operator_params() : repair_operator_params;
  for (const auto& operator_param : repair_params) {
    create_repair_operator(alns_operator_manager.get(), operator_param, context, parallel_executor);
  }
  // 初始化权重与统计矩阵
  alns_operator_manager->init_weights();
  return alns_operator_manager;
}
