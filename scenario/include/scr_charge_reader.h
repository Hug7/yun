/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <spdlog/spdlog.h>

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "bdm_charge.h"

/**
 * @brief 计费表读取器
 * @details
 * 只负责读取、解析与校验，读取完成后调用 ChargeManager::build_indexes 建立运行时索引。
 * 读取阶段允许使用 unordered_map 做字符串到整数索引的冷路径解析。
 */
class ChargeCsvReader {
 public:
  ChargeCsvReader(std::string root_dir, std::shared_ptr<spdlog::logger> logger,
                  DimensionManager* dim_manager, LabelManager* label_manager,
                  CarrierManager* carrier_manager, VehicleModelManager* vehicle_model_manager,
                  LocationManager* location_manager);

  ~ChargeCsvReader() = default;

  /**
   * @brief 读取计费表
   * @details 场景目录不存在 ChargeRule.csv 时返回一个未启用的 ChargeManager
   */
  [[nodiscard]] ChargeManager* load() const;

 private:
  /**
   * @brief 阶梯草稿
   */
  struct TierDraft {
    int seq{0};
    long lower{0};
    long upper{0};
    std::vector<ChargeItem> items;
    size_t row{0};
  };

  /**
   * @brief 规则草稿
   */
  struct RuleDraft {
    std::string rule_code;
    ChargeRule rule;
    std::vector<TierDraft> tiers;
  };

  const std::string root_dir;
  const std::shared_ptr<spdlog::logger> logger;
  DimensionManager* dim_manager;
  LabelManager* label_manager;
  CarrierManager* carrier_manager;
  VehicleModelManager* vehicle_model_manager;
  LocationManager* location_manager;

  void load_charge_codes(ChargeManager* manager,
                         std::unordered_map<std::string, int>& charge_code_index) const;

  void load_rules(ChargeManager* charge_manager,
                  const std::unordered_map<std::string, int>& charge_code_index,
                  std::unordered_map<std::string, int>& rule_index,
                  std::vector<RuleDraft>& rule_drafts) const;

  void load_tiers(std::vector<RuleDraft>& rule_drafts,
                  const std::unordered_map<std::string, int>& rule_index) const;

  void load_items(std::vector<RuleDraft>& rule_drafts,
                  const std::unordered_map<std::string, int>& rule_index) const;

  void finalize(ChargeManager* manager, std::vector<RuleDraft>& rule_drafts) const;
};
