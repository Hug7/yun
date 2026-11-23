/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <cstdint>

#include "hc_constraint_code.h"
#include "pdm_load.h"
#include "pdm_score.h"

class Segment;
class VehicleResource;

/**
 * @brief 硬约束预检结论
 */
enum class HcPrecheckVerdict : std::uint8_t {
  /**
   * @brief 必定不违背该约束
   * @details 仅表示该约束本身不会被违背, 不能据此跳过其它约束
   */
  kPass,
  /**
   * @brief 必定违背该约束
   * @details 该车次在插入该 segment 后不可能可行, 可以直接剪枝
   */
  kInfeasible,
  /**
   * @brief 无法预检
   * @details 交回 eval 做完整判定
   */
  kUnknown,
};

/**
 * @brief 硬约束预检输入
 * @details 描述"把 segment 插入 load"这个动作, 不携带具体插入位置:
 * 与插入位置无关的约束才可以在这里给出确定结论。
 */
struct HcPrecheckInput {
  /**
   * @brief 插入的基车次
   */
  Load* load;
  /**
   * @brief 待插入的订单片段
   */
  const Segment* segment;
  /**
   * @brief 车辆资源, 用于判断换车型后是否仍不可行
   */
  const VehicleResource* vehicle_resource;
};

/**
 * @brief 硬约束父类
 */
class HardConstraint {
 public:
  /**
   * @brief 硬约束编码
   */
  const HardConstrCode code;
  /**
   * @brief 约束的执行优先级
   * @details priority=0时，代表最后执行
   */
  const int priority;
  /**
   * @brief 是否为受顺序影响的约束
   */
  const bool seq_type;
  /**
   * @brief 是否为受车辆影响的约束
   */
  const bool vehicle_type;

  HardConstraint(const HardConstrCode code, const int priority, const bool seq_type,
                 const bool vehicle_type)
      : code(code), priority(priority), seq_type(seq_type), vehicle_type(vehicle_type) {};

  virtual ~HardConstraint() = default;

  /**
   * @brief 评价硬约束
   */
  virtual HardConstrScore::UPtr eval(Load* load) = 0;

  /**
   * @brief 预检硬约束
   * @details 只在能给出确定结论时返回 kPass 或 kInfeasible, 否则返回 kUnknown 由 eval 兜底。
   * 默认不实现预检: 子类覆盖本方法即开启该约束的预检。
   */
  [[nodiscard]] virtual HcPrecheckVerdict precheck(const HcPrecheckInput& input) const {
    static_cast<void>(input);
    return HcPrecheckVerdict::kUnknown;
  }
};
