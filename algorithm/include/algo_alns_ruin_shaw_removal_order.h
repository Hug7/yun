/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <cstddef>
#include <functional>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "algo_alns_ruin.h"
#include "c_random_utils.h"

class OrderRelationshipGraph;

/**
 * @brief 订单对之间的原始相近度分量
 * @details first 为空间距离(不可拼车时为无穷大), second 为时间距离; 两者都是越小越相近
 */
using ShawOrderDistanceFunc = std::function<std::pair<double, double>(int, int)>;

/**
 * @brief 把一组原始值做 min-max 归一化到 `[0, 1]`
 * @details 非有限值(如不可拼车的无穷大)归一化为 `1.0`; 没有有限值或有限值全部相等时返回全 `0`
 * (无穷大值仍为 `1.0`)
 * @param values 原始值
 * @return 与 `values` 等长的归一化结果
 */
std::vector<double> min_max_normalize(const std::vector<double>& values);

/**
 * @brief 按 Shaw 相近度挑选要移除的订单
 * @details 先随机挑一个种子订单; 每轮把候选订单到「已选集合」的空间、时间最小距离分别在候选池内
 * 做 min-max 归一化, 按权重线性组合后升序排序, 再按 `pos = floor(|pool| * y^p)`, `y~U(0,1)` 取位;
 * 重复直到选满 `count` 或候选耗尽。空间/时间权重为 0 时对应项不参与排序。
 * @param order_count 候选订单数
 * @param order_distance (i, j) -> (空间距离, 时间距离), i/j 为 `[0, order_count)` 的局部下标
 * @param count 期望移除的订单数(含种子订单), 超过候选数时截断
 * @param spatial_weight 空间项权重, 需 `>= 0`
 * @param time_weight 时间项权重, 需 `>= 0`, 且与空间权重不可同时为 0
 * @param noise_exponent 贪心指数 p, 需 `>= 1`; 越大越集中在最相近的订单上
 * @param random 随机源
 * @return 被选中订单的局部下标, 按挑选顺序(首个为种子订单)
 */
std::vector<int> select_shaw_removal_order_indices(int order_count,
                                                   const ShawOrderDistanceFunc& order_distance,
                                                   int count, double spatial_weight,
                                                   double time_weight, double noise_exponent,
                                                   RandomUtils& random);

/**
 * @brief alns ruin算子`RuinShawRemovalOrder`的参数
 */
struct AlnsRuinShawRemovalOrderParam {
  /**
   * @brief 随机种子
   */
  int select_order_random_seed{37};
  /**
   * @brief 选择订单数的最小比例
   */
  double select_order_min_rate{0.1};
  /**
   * @brief 选择订单数的最大比例
   */
  double select_order_max_rate{0.3};
  /**
   * @brief 贪心指数 p
   * @details 1 表示在候选池上均匀随机, 越大越集中在最相近的订单上
   */
  double select_order_noise_exponent{3.0};
  /**
   * @brief 空间相近度权重
   */
  double select_order_spatial_weight{0.5};
  /**
   * @brief 时间相近度权重
   */
  double select_order_time_weight{0.5};
};

/**
 * @brief alns ruin算子 Shaw 订单移除
 * @details 以「订单对的空间距离 + 时间窗间隙」作为相近度, 从随机种子出发迭代移除与已选集合最相近的
 * 订单, 使一次移除的订单在地理区域与服务时间上彼此相近
 */
class AlnsRuinShawRemovalOrder : public AlnsRuinOperator {
 public:
  /**
   * @brief 结构化参数
   */
  const AlnsRuinShawRemovalOrderParam parameter;
  /**
   * @brief 订单关系图, 可为空; 为空时空间项按 0 处理(等价于只用时间项)
   */
  const OrderRelationshipGraph* order_relationship_graph;
  /**
   * @brief 随机数
   */
  RandomUtils::UPtr select_order_random;

  explicit AlnsRuinShawRemovalOrder(const SolverContext* _context,
                                    const OrderRelationshipGraph* _order_relationship_graph,
                                    const AlnsRuinShawRemovalOrderParam& _parameter);

  [[nodiscard]] std::string format_parameters() const override;

  std::unordered_map<int, AlnsRuinLoadActivity::UPtr> execute(Solution* sol) override;
};
