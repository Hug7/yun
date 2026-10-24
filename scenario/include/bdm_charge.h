/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <limits>
#include <string>
#include <vector>

#include "bdm_carrier.h"
#include "bdm_dimension.h"
#include "bdm_label.h"
#include "bdm_location.h"
#include "bdm_vehicle_model.h"

namespace ChargeParameter {
/**
 * @brief 金额定点精度：1e-4 元
 */
constexpr long MONEY_SCALE = 10000;
/**
 * @brief 1 km = 1000 m
 */
constexpr long METER_PER_KM = 1000;
}  // namespace ChargeParameter

/**
 * @brief 计费线路类型
 */
enum class ChargePathType {
  /**
   * @brief 不限制线路
   */
  NONE = 0,
  /**
   * @brief 起讫点对，例如 上海-苏州
   */
  OD_PAIR = 1,
  /**
   * @brief 有序线路，例如 上海-苏州-无锡
   */
  ORDERED_ROUTE = 2,
};

/**
 * @brief 阶梯查询条件类型
 */
enum class ChargeConditionType {
  NONE = 0,             // 不限制
  DISTANCE = 1,         // 距离
  PICK_NODE_COUNT = 2,  // 提货点数
  DROP_NODE_COUNT = 3,  // 卸货点数
  DIMENSION = 4,        // 维度
};

/**
 * @brief 计费项类型
 */
enum class ChargeItemType {
  FLAT = 0,                // Price
  DISTANCE = 1,            // Price * 里程(km)
  PICK_STOP = 2,           // Price * max(0, 提货节点数 − 1)
  DROP_STOP = 3,           // Price * max(0, 卸货节点数 − 1)
  DIMENSION = 4,           // Price * 维度总量(自然单位)
  DIMENSION_DISTANCE = 5,  // Price * 维度总量(自然单位) × 里程(km)
};

/**
 * @brief 计费阶梯
 * @details 区间为 [lower, upper)，upper 为最大值时表示正无穷
 */
struct ChargeTier {
  /**
   * @brief 下界（含），内部单位
   */
  long lower;
  /**
   * @brief 上界（不含），内部单位
   */
  long upper;
  /**
   * @brief 计费项在 ChargeManager::items 中的起始位置
   */
  int item_begin;
  /**
   * @brief 计费项数量
   */
  int item_len;
};

/**
 * @brief 计费项
 */
struct ChargeItem {
  /**
   * @brief 计费项类型
   */
  ChargeItemType type;
  /**
   * @brief 单价，单位：1e-4 元/计价单位
   */
  long price;
  /**
   * @brief 维度索引，非维度计费项为 -1
   */
  int dim_ind;
};

/**
 * @brief 计费规则
 * @details 字符串编码已在读取阶段解析为整数索引，热路径不再做字符串比较
 */
struct ChargeRule {
  /**
   * @brief 计费编码索引
   */
  int charge_code_ind{-1};
  /**
   * @brief 次级编码索引
   */
  int secondary_ind{-1};
  /**
   * @brief 承运商索引，-1 表示任意
   */
  int carrier_ind{-1};
  /**
   * @brief 车型索引，-1 表示任意
   */
  int vehicle_model_ind{-1};
  /**
   * @brief 线路类型
   */
  ChargePathType path_type{ChargePathType::NONE};
  /**
   * @brief 线路标签在 location labelset 中的索引，-1 表示不使用
   */
  int path_label_ind4labelset{-1};
  /**
   * @brief 线路标签值索引序列
   */
  std::vector<int> path_values;
  /**
   * @brief 阶梯查询条件
   */
  ChargeConditionType condition_type{ChargeConditionType::NONE};
  /**
   * @brief 阶梯条件使用的维度索引，-1 表示不使用
   */
  int condition_dim_ind{-1};
  /**
   * @brief 线路标签在 ChargeManager::path_label_inds 中的槽位
   */
  int path_label_slot{-1};
  /**
   * @brief 阶梯在 ChargeManager::tiers 中的起始位置
   */
  int tier_begin{0};
  /**
   * @brief 阶梯数量
   */
  int tier_len{0};
};

/**
 * @brief 车次计费视图
 * @details
 * 只暴露计费需要的车次事实，避免 scenario 层依赖 problem 层的 Load。
 * 所有字段都按需加载：ChargeManager 只在规则真正需要某项数据时调用对应 getter，
 * 具体实现可以缓存结果，避免重复遍历节点链。
 */
class ChargeLoadView {
 public:
  virtual ~ChargeLoadView() = default;

  /**
   * @brief 车辆索引
   */
  [[nodiscard]] virtual int vehicle_ind() const = 0;

  /**
   * @brief 车次总里程（米）
   */
  [[nodiscard]] virtual long distance_m() const = 0;

  /**
   * @brief 提货节点数
   */
  [[nodiscard]] virtual int pick_node_count() const = 0;

  /**
   * @brief 卸货节点数
   */
  [[nodiscard]] virtual int drop_node_count() const = 0;

  /**
   * @brief 车次维度总量（原始值）
   */
  [[nodiscard]] virtual const long* dim_vals() const = 0;

  /**
   * @brief 车次停靠点（按顺序，不含 depot 哨兵）
   */
  [[nodiscard]] virtual const Location* const* stops() const = 0;

  /**
   * @brief 停靠点数量
   */
  [[nodiscard]] virtual int stop_count() const = 0;

  /**
   * @brief 订单提货站点
   */
  [[nodiscard]] virtual const Location* const* order_pick_locs() const = 0;

  /**
   * @brief 订单卸货站点
   */
  [[nodiscard]] virtual const Location* const* order_drop_locs() const = 0;

  /**
   * @brief 订单数量
   */
  [[nodiscard]] virtual int order_count() const = 0;
};

class ChargeEvaluator;

/**
 * @brief 计费结果
 */
struct ChargeResult {
  /**
   * @brief 运费，单位：1e-4 元
   */
  long amount{0};
  /**
   * @brief 是否至少有一个计费编码可以计费
   * @details false 表示没有任何条款匹配、或所有条款都未通过 OD 覆盖检查
   */
  bool matched{false};
};

/**
 * @brief 计费管理器
 * @details
 * 持有计费表数据与运行时索引。设计目标是热路径（ChargeManager::evaluate）
 * 不做字符串比较、不查 unordered_map、不产生每规则小分配。
 */
class ChargeManager {
 public:
  /**
   * @brief 是否启用计费（存在至少一条规则）
   */
  bool enabled{false};
  /**
   * @brief 维度数量
   */
  int dim_len{0};

  /**
   * @brief 计费编码列表
   */
  std::vector<std::string> charge_code_codes;
  /**
   * @brief 计费编码名称列表
   */
  std::vector<std::string> charge_code_names;
  /**
   * @brief 次级编码所属的计费编码索引
   */
  std::vector<int> secondary_charge_code_inds;
  /**
   * @brief 次级编码列表
   */
  std::vector<std::string> secondary_codes;

  /**
   * @brief 规则列表
   */
  std::vector<ChargeRule> rules;
  /**
   * @brief 阶梯列表（按规则连续存放）
   */
  std::vector<ChargeTier> tiers;
  /**
   * @brief 计费项列表（按阶梯连续存放）
   */
  std::vector<ChargeItem> items;

  ChargeManager() = default;

  ~ChargeManager() = default;

  /**
   * @brief 添加次级编码
   * @param charge_code_ind 计费编码索引
   * @param secondary_code 次级编码
   * @return 次级编码索引；已存在时返回已有索引
   */
  int add_secondary(int charge_code_ind, const std::string& secondary_code);

  /**
   * @brief 建立运行时索引
   * @details 读取完成后调用一次，之后 ChargeManager 为只读对象
   */
  void build_indexes(const DimensionManager* dim_manager, const LabelManager* label_manager,
                     const CarrierManager* carrier_manager,
                     const VehicleModelManager* vehicle_model_manager,
                     const LocationManager* location_manager);

  /**
   * @brief 计算一个车次的运输成本
   * @param view 车次计费视图
   * @return 计费结果（金额 + 是否匹配到可计费条款）
   */
  [[nodiscard]] ChargeResult evaluate(const ChargeLoadView& view) const;

 private:
  // 仅用于 bdm_charge_eval.cpp 内部的求值器，避免在头文件暴露实现细节
  friend class ChargeEvaluator;

  /**
   * @brief 规则用到的线路标签槽位
   */
  std::vector<int> path_label_inds;
  /**
   * @brief 站点标签值 CSR 偏移
   * @details 下标为 location_ind * path_label_count + label_slot
   */
  std::vector<int> loc_label_offsets;
  /**
   * @brief 站点标签值 CSR 数据
   */
  std::vector<int> loc_label_values;
  /**
   * @brief 被 OD_PAIR 规则使用的线路标签槽位
   */
  std::vector<int> od_path_label_slots;
  /**
   * @brief 被 ORDERED_ROUTE 规则使用的线路标签槽位
   */
  std::vector<int> ordered_path_label_slots;
  /**
   * @brief 每个次级编码的 OD 线路标签槽位，-1 表示该次级编码没有 OD 规则
   * @details 同一 (ChargeCode, ChargeSecondaryCode) 下的 OD 规则必须使用同一标签
   */
  std::vector<int> secondary_od_slot;
  /**
   * @brief 含 OD 规则的次级编码索引
   */
  std::vector<int> od_secondary_inds;
  /**
   * @brief 每辆车的专用规则在 vehicle_rule_inds 中的区间
   */
  std::vector<int> vehicle_rule_offsets;
  /**
   * @brief 按车辆预筛后的专用规则索引
   */
  std::vector<int> vehicle_rule_inds;
  /**
   * @brief 不限定承运商/车型的规则索引
   */
  std::vector<int> generic_rule_inds;
  /**
   * @brief 维度精度对应的 10^precision
   */
  std::vector<long> dim_scales;
  /**
   * @brief 站点数量
   */
  int location_count{0};
  /**
   * @brief 线路标签槽位数量
   */
  int path_label_count{0};
};
