/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <variant>
#include <vector>

#include "algo_ls_exchange.h"
#include "algo_ls_parameter.h"
#include "algo_ls_segment_relocate.h"
#include "algo_ls_two_opt.h"
#include "algo_ls_two_opt_star.h"
#include "pdm_graph_order.h"

class LsOperator;
class Solution;
class SolverContext;

/**
 * @brief 邻域算子参数（每个分支对应一种算子实例）
 */
using LsOperatorParam =
    std::variant<LsSegmentRelocateParam, LsExchangeParam, LsTwoOptParam, LsTwoOptStarParam>;

/**
 * @brief 一次邻域搜索的配置
 * @details algorithm 层自包含；solver 层的 Lua 解析结果直接使用本类型（可 alias 为
 * LsStrategyConfig）， 避免在 algorithm/solver 各定义一份同构结构。
 */
struct LsSearchConfig {
  LsParameter parameter;
  std::vector<LsOperatorParam> operators;
};

/**
 * @brief 按算子实例累计的统计
 */
struct LsOperatorStat {
  std::string code;
  int instance_index{0};
  int generated{0};
  ///< KNN 结构剪枝掉的候选数
  int pruned{0};
  ///< 里程增量剪枝掉的候选数
  int dist_pruned{0};
  int evaluated{0};
  int feasible{0};
  int applied{0};
  double delta_obj{0};
};

/**
 * @brief 一次邻域搜索的报告
 */
struct LsReport {
  double initial_obj{0};
  double final_obj{0};
  double delta_obj{0};
  int passes{0};
  int generated{0};
  ///< KNN 结构剪枝掉的候选数
  int pruned{0};
  ///< 里程增量剪枝掉的候选数
  int dist_pruned{0};
  int evaluated{0};
  int feasible{0};
  int applied{0};
  std::int64_t elapsed_ms{0};
  /**
   * @brief 本次搜索使用的改进策略：first / best
   */
  std::string improvement{"best"};
  /**
   * @brief no_improvement / no_candidate / max_evaluated
   */
  std::string termination_reason;
  std::vector<LsOperatorStat> operator_stats;

  /**
   * @brief 单行摘要 + 各算子实例统计，不含结尾换行
   */
  [[nodiscard]] std::string format() const;
};

/**
 * @brief 默认 7 个邻域实例的参数：同车/跨车的 relocate、or-opt、exchange，加 2-opt*
 */
[[nodiscard]] std::vector<LsOperatorParam> default_ls_operator_params();

/**
 * @brief 按参数创建算子实例；参数为空时使用默认集
 */
[[nodiscard]] std::vector<std::unique_ptr<LsOperator>> create_ls_operators(
    const std::vector<LsOperatorParam>& operator_params);

/**
 * @brief 在 sol 上就地执行邻域搜索
 * @details 全量搜索：不设时间预算、move 数或候选上限（可用 `max_evaluated` 给评估次数封顶），
 *          直到一整个 pass 都没有改进。
 *          改进策略由 `LsParameter::improvement` 决定：`Best` 扫完本 pass 全部候选后只提交
 *          最优的改进 move，`First` 遇到第一个改进候选就提交并结束本 pass；每个 pass 的
 *          起点算子按 pass 序号轮转。评估前有两层剪枝：先按 KNN 图过滤新建的相邻关系，
 *          再按里程增量过滤会增加车次里程的候选（涉事车次不可行时不剪）。
 * @param order_relationship_graph 订单关系图，用于构建 KNN 剪枝图；传 nullptr 或让
 *                  `knn_neighbor_count` 为 0 都表示不做 KNN 剪枝。
 *                  近邻数取 `LsParameter::knn_neighbor_count`；图由 `LsSearch` 持有，
 *                  首次搜索时构建一次，多次搜索复用。
 * @pre   sol 非空；所有 Load 非空（get_order_count() > 0）且已 eval_load 过（可行）
 * @pre   context->problem 非空；context->problem->pd_pattern 支持 LS（v1 仅 SPMD）
 * @post  sol->total_obj_val() 不增；loads.size()、各 load->vehicle、unassigned_orders、
 *        orders_view 不变；所有 Load 可行且非空
 * @throws std::invalid_argument 前置条件不满足 / 参数非法 / pattern 不支持
 */
[[nodiscard]] LsReport run_local_search(Solution* sol, const LsParameter& parameter,
                                        const std::vector<LsOperatorParam>& operator_params,
                                        const SolverContext* context,
                                        const OrderRelationshipGraph* order_relationship_graph);
