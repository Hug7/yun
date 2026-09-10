/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#include "pdm_context.h"

#include <format>
#include <random>

#include "c_chrono_util.h"
#include "cc_dist.h"
#include "hc_label.h"
#include "hc_location.h"
#include "hc_vehicle.h"
#include "sc_dist.h"
#include "scr_standard_csv_reader.h"

/**
 * @brief 生成 UUID v4，格式 xxxxxxxx-xxxx-4xxx-yxxx-xxxxxxxxxxxx
 */
std::string generate_uuid() {
  static thread_local std::mt19937_64 gen(std::random_device{}());
  const uint64_t hi = gen();
  const uint64_t lo = gen();

  // UUID v4：版本号固定为 4，variant 高 2 位固定为 10
  const auto time_low = static_cast<uint32_t>(hi >> 32);
  const auto time_mid = static_cast<uint32_t>((hi >> 16) & 0xFFFF);
  const auto time_hi = static_cast<uint32_t>(((hi >> 48) & 0x0FFF) | 0x4000);
  const auto clock_seq = static_cast<uint32_t>(((lo >> 48) & 0x3FFF) | 0x8000);
  const auto node_hi = static_cast<uint32_t>((lo >> 32) & 0xFFFF);
  const auto node_low = static_cast<uint32_t>(lo);

  return std::format("{:08x}-{:04x}-{:04x}-{:04x}-{:04x}{:08x}", time_low, time_mid, time_hi,
                     clock_seq, node_hi, node_low);
}  // namespace

/**
 * @brief 生成唯一的请求ID，格式 %Y%m%d%H%M%S_{uuid}
 */
std::string generate_request_id() {
  // return chrono_util::format(chrono_util::now(), "%Y%m%d%H%M%S") + "_" + generate_uuid();
  return chrono_util::format(chrono_util::now(), "%Y%m%d%H%M%S");
}

/**
 * 读取场景
 * @param root_dir 根目录
 * @return 场景
 */
Scenario* load_scenario(const std::string& root_dir) {
  const auto reader = std::make_unique<StandardCsvReader>(root_dir);
  Scenario* scenario = reader->loading_scenario();
  return scenario;
}

/**
 * @brief 读取参数
 * @param scenario 场景
 */
Parameter* load_parameter(const Scenario* scenario) {
  Parameter* parameter = new Parameter(scenario->label_manager->order_labelset);
  // TODO 实现读取参数

  // 参数后处理
  parameter->post_process(scenario);

  return parameter;
}
/**
 * @brief 创建问题
 * @param scenario 场景
 * @param parameter 参数
 */
Problem* create_problem(const Scenario* scenario, Parameter* parameter) {
  Problem* problem = new Problem(scenario, parameter);

  // hard constraints: base
  problem->hc_manager->add_constr(new HcVehicleCapacity());
  problem->hc_manager->add_constr(new HcAvailableVehicle());

  // add hard constraints by scenario
  // -- hard constraints: max pick node count
  if (parameter->time_window_constr_enabled) {
    problem->hc_manager->add_constr(new HcTimeWindow());
  }
  // -- hard constraints: max drop node count
  if (parameter->max_pick_node_count > HardConstraintParameter::DEFAULT_MAX_PICK_NODE_COUNT) {
    problem->hc_manager->add_constr(new HcMaxPickNodeCount(parameter->max_pick_node_count));
  }
  // -- hard constraints: max pick node count
  if (parameter->max_drop_node_count > HardConstraintParameter::DEFAULT_MAX_DROP_NODE_COUNT) {
    problem->hc_manager->add_constr(new HcMaxDropNodeCount(parameter->max_drop_node_count));
  }
  // soft constraints
  if (parameter->sc_constr_dist_factor > SoftConstraintParameter::SC_DIST_DEFAULT_DIST_FACTOR) {
    problem->sc_manager->add_constr(new ScDist(parameter->sc_constr_dist_factor));
  }
  // cost constraints
  // -- cost constraints: dist
  if (parameter->cc_constr_dist_factor > CostConstraintParameter::CC_DIST_DEFAULT_DIST_FACTOR) {
    problem->cc_manager->add_constr(new CcDist(parameter->cc_constr_dist_factor));
  }

  return problem;
}

// ====== implement of Load CcDist ======
SolverContext::SolverContext(std::string root_dir)
    : root_dir(std::move(root_dir)), request_id(generate_request_id()) {
  // 读取场景
  this->scenario = load_scenario(this->root_dir);
  // 读取参数
  this->parameter = load_parameter(this->scenario);
  // 可视化管理器
  this->visual_manager = new VisualManager(this->root_dir, this->request_id);
  // 创建问题
  this->problem = create_problem(this->scenario, this->parameter);
}

SolverContext::~SolverContext() {
  delete this->scenario;
  delete this->parameter;
  delete this->visual_manager;
  delete this->problem;
}
